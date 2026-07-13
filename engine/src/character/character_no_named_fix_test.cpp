#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#ifndef GHOGX_CHARACTER_SOURCE_DIR
#define GHOGX_CHARACTER_SOURCE_DIR "."
#endif

namespace {

std::string read_file(const std::filesystem::path& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    throw std::runtime_error("failed to open " + path.string());
  }
  std::ostringstream ss;
  ss << in.rdbuf();
  return ss.str();
}

std::string strip_comments_keep_strings(const std::string& input) {
  std::string out;
  out.reserve(input.size());
  bool in_line_comment = false;
  bool in_block_comment = false;
  bool in_string = false;
  bool in_char = false;
  bool escaped = false;

  for (size_t i = 0; i < input.size(); ++i) {
    const char c = input[i];
    const char next = (i + 1 < input.size()) ? input[i + 1] : '\0';

    if (in_line_comment) {
      if (c == '\n') {
        in_line_comment = false;
        out.push_back(c);
      }
      continue;
    }
    if (in_block_comment) {
      if (c == '*' && next == '/') {
        in_block_comment = false;
        ++i;
      } else if (c == '\n') {
        out.push_back(c);
      }
      continue;
    }
    if (!in_string && !in_char && c == '/' && next == '/') {
      in_line_comment = true;
      ++i;
      continue;
    }
    if (!in_string && !in_char && c == '/' && next == '*') {
      in_block_comment = true;
      ++i;
      continue;
    }

    out.push_back(c);
    if (in_string || in_char) {
      if (escaped) {
        escaped = false;
      } else if (c == '\\') {
        escaped = true;
      } else if (in_string && c == '"') {
        in_string = false;
      } else if (in_char && c == '\'') {
        in_char = false;
      }
      continue;
    }
    if (c == '"') {
      in_string = true;
    } else if (c == '\'') {
      in_char = true;
    }
  }
  return out;
}

std::string lowercase(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return s;
}

}  // namespace

int main() {
  const std::filesystem::path source_dir = GHOGX_CHARACTER_SOURCE_DIR;
  const std::vector<std::filesystem::path> files = {
      "char_clip.cpp",     "char_clip.h",     "char_facefx.cpp",
      "char_facefx.h",     "char_mesh.cpp",   "char_mesh.h",
      "char_renderer.cpp", "char_renderer.h",
  };
  const std::vector<std::string> forbidden = {
      "glam1",       "rock2",     "metal_bass", "deathmetal1",
      "deathmetal",  "rockabill1", "rockabill",  "goth3",
      "terminal_leg_overlay_duplicate",
      "ankle_toe_palette",
      "hidden_numbered_hair_variant",
      "hair_numbered_variant",
  };

  bool ok = true;
  for (const auto& rel : files) {
    const auto path = source_dir / rel;
    const std::string scanned = lowercase(strip_comments_keep_strings(read_file(path)));
    for (const auto& token : forbidden) {
      if (scanned.find(token) == std::string::npos) continue;
      std::cerr << "Forbidden character-specific runtime token '" << token
                << "' in " << path.string() << "\n";
      ok = false;
    }
  }
  const auto char_clip_path = source_dir / "char_clip.cpp";
  const std::string char_clip =
      lowercase(strip_comments_keep_strings(read_file(char_clip_path)));
  const std::vector<std::string> forbidden_default_on_switches = {
      "ghogx_disable_charbone_lower_body_output",
      "ghogx_disable_charbone_output_layer",
      "ghogx_disable_charbone_face_output",
  };
  for (const auto& token : forbidden_default_on_switches) {
    if (char_clip.find(token) == std::string::npos) continue;
    std::cerr << "Forbidden default-on CharBone output switch '" << token
              << "' in " << char_clip_path.string() << "\n";
    ok = false;
  }

  const std::vector<std::string> forbidden_removed_scaffold = {
      "apply_ps2_ik_hand_targets",
      "ps2_ordered_ik_hands",
      "classify_ps2_ik_poll_role",
      "ps2ikpollrole",
      "ps2_ik_hand_position_enabled",
      "ps2_ik_hand_final_disabled",
      "fore_twists_applied",
  };
  for (const auto& token : forbidden_removed_scaffold) {
    if (char_clip.find(token) == std::string::npos) continue;
    std::cerr << "Forbidden removed hand/IK scaffold token '" << token
              << "' in " << char_clip_path.string() << "\n";
    ok = false;
  }

  const std::vector<std::string> required_opt_in_switches = {
      "ghogx_enable_charbone_lower_body_output",
      "ghogx_enable_charbone_output_layer",
      "ghogx_enable_charbone_face_output",
  };
  for (const auto& token : required_opt_in_switches) {
    if (char_clip.find(token) != std::string::npos) continue;
    std::cerr << "Missing explicit opt-in CharBone output switch '" << token
              << "' in " << char_clip_path.string() << "\n";
    ok = false;
  }
  if (!ok) {
    std::cerr << "Broken outfits and broad CharBone output must stay on shared "
                 "source-backed paths, not named branches or promoted "
                 "diagnostic scaffolding.\n";
    return 1;
  }
  return 0;
}
