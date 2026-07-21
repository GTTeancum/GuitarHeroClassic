#include "ui/pss_video_player_win32.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>

namespace ghogx::ui {

namespace {

std::string quote_command_arg(const std::string& value) {
  std::string out = "\"";
  std::size_t slashes = 0;
  for (char ch : value) {
    if (ch == '\\') {
      ++slashes;
      continue;
    }
    if (ch == '"') {
      out.append(slashes * 2 + 1, '\\');
      out.push_back('"');
      slashes = 0;
      continue;
    }
    out.append(slashes, '\\');
    slashes = 0;
    out.push_back(ch);
  }
  out.append(slashes * 2, '\\');
  out.push_back('"');
  return out;
}

}  // namespace

PssVideoPlayerWin32::~PssVideoPlayerWin32() { close(); }

bool PssVideoPlayerWin32::open(const std::string& path) {
  close();
  finished_ = false;
  if (path.empty() || !std::filesystem::exists(path)) {
    std::fprintf(stderr, "[boot-video] source not found: %s\n", path.c_str());
    finished_ = true;
    return false;
  }

  SECURITY_ATTRIBUTES security{};
  security.nLength = sizeof(security);
  security.bInheritHandle = TRUE;
  HANDLE read_pipe = nullptr;
  HANDLE write_pipe = nullptr;
  if (!CreatePipe(&read_pipe, &write_pipe, &security, 4u * 1024u * 1024u)) {
    std::fprintf(stderr, "[boot-video] CreatePipe failed: %lu\n",
                 static_cast<unsigned long>(GetLastError()));
    finished_ = true;
    return false;
  }
  SetHandleInformation(read_pipe, HANDLE_FLAG_INHERIT, 0);

  HANDLE null_handle = CreateFileA(
      "NUL", GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, &security,
      OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
  STARTUPINFOA startup{};
  startup.cb = sizeof(startup);
  startup.dwFlags = STARTF_USESTDHANDLES;
  startup.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
  startup.hStdOutput = write_pipe;
  startup.hStdError = null_handle != INVALID_HANDLE_VALUE
                             ? null_handle
                             : GetStdHandle(STD_ERROR_HANDLE);

  // The retail intro is 640x448 at 30000/1001.  Force that decoded contract so
  // the render loop never has to infer dimensions from the proprietary suffix.
  std::string command =
      "ffmpeg.exe -hide_banner -loglevel error -nostdin -re -i " +
      quote_command_arg(path) +
      " -map 0:v:0 -an -sn -vf scale=640:448,fps=30000/1001 "
      "-pix_fmt rgba -f rawvideo pipe:1";
  std::vector<char> mutable_command(command.begin(), command.end());
  mutable_command.push_back('\0');

  PROCESS_INFORMATION process{};
  const BOOL created = CreateProcessA(
      nullptr, mutable_command.data(), nullptr, nullptr, TRUE,
      CREATE_NO_WINDOW, nullptr, nullptr, &startup, &process);
  CloseHandle(write_pipe);
  if (null_handle != INVALID_HANDLE_VALUE) CloseHandle(null_handle);
  if (!created) {
    CloseHandle(read_pipe);
    std::fprintf(stderr,
                 "[boot-video] ffmpeg decoder unavailable (error %lu)\n",
                 static_cast<unsigned long>(GetLastError()));
    finished_ = true;
    return false;
  }
  CloseHandle(process.hThread);
  process_ = process.hProcess;
  read_pipe_ = read_pipe;
  rgba_.resize(static_cast<std::size_t>(width_) *
               static_cast<std::size_t>(height_) * 4u);
  active_ = true;
  std::fprintf(stderr, "[boot-video] playing source PSS: %s\n", path.c_str());
  return true;
}

bool PssVideoPlayerWin32::read_next_frame() {
  if (!active_ || !read_pipe_ || rgba_.empty()) return false;
  HANDLE pipe = static_cast<HANDLE>(read_pipe_);
  std::size_t offset = 0;
  while (offset < rgba_.size()) {
    DWORD got = 0;
    const DWORD request = static_cast<DWORD>(
        std::min<std::size_t>(rgba_.size() - offset, 1u << 20));
    if (!ReadFile(pipe, rgba_.data() + offset, request, &got, nullptr) ||
        got == 0) {
      close();
      finished_ = true;
      return false;
    }
    offset += got;
  }
  return true;
}

void PssVideoPlayerWin32::close() {
  if (read_pipe_) {
    CloseHandle(static_cast<HANDLE>(read_pipe_));
    read_pipe_ = nullptr;
  }
  if (process_) {
    HANDLE process = static_cast<HANDLE>(process_);
    if (WaitForSingleObject(process, 0) == WAIT_TIMEOUT) {
      TerminateProcess(process, 0);
      WaitForSingleObject(process, 1000);
    }
    CloseHandle(process);
    process_ = nullptr;
  }
  active_ = false;
}

}  // namespace ghogx::ui
