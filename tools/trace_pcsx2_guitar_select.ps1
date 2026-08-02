param(
  [string]$Pcsx2 = "engine\out\tools\pcsx2-silo\pcsx2-v2.7.93-local\pcsx2-qt.exe",
  [string]$Iso = "C:\Programming\GitHub\Guitar Hero II\Guitar Hero II PS2 (USA).iso",
  [string]$OutDir = "engine\out\menu_tuning\pcsx2\silo_guitar_select_logic_trace_20260703",
  [int]$SetupCrossCount = 4,
  [int]$PostCareerCrossCount = -1,
  [switch]$FreshBoot,
  [switch]$SkipHook,
  [switch]$CaptureSteps,
  [switch]$CareerTrace,
  [ValidateSet("Hidden", "Minimized")]
  [string]$OracleWindowStyle = "Hidden",
  [switch]$DisarmTogglePauseHotkey,
  [int]$StateSlot = 1,
  [switch]$TraceCurrentScreen,
  [int]$CurrentScreenCrossCount = 0,
  [switch]$DeclineSaveAfterCurrentCrosses,
  [switch]$NoProofCapture
)

$ErrorActionPreference = "Stop"

$Pcsx2 = (Resolve-Path -LiteralPath $Pcsx2).Path
$Repo = (Resolve-Path -LiteralPath ".").Path
$OutDir = Join-Path $Repo $OutDir
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null
$log = Join-Path $OutDir "pcsx2.log"
$traceJson = Join-Path $OutDir "guitar_select_logic_trace.json"
$proofPng = Join-Path $OutDir "guitar_select_settled.printwindow.png"
$snapProofPng = Join-Path $OutDir "guitar_select_settled.pcsx2snap.png"
$traceStart = Get-Date
Remove-Item -LiteralPath $log -Force -ErrorAction SilentlyContinue
Remove-Item -LiteralPath $traceJson -Force -ErrorAction SilentlyContinue
Remove-Item -LiteralPath $proofPng -Force -ErrorAction SilentlyContinue
Remove-Item -LiteralPath $snapProofPng -Force -ErrorAction SilentlyContinue
Get-ChildItem -LiteralPath $OutDir -File -Filter "*.pcsx2snap.png" -ErrorAction SilentlyContinue |
  Remove-Item -Force -ErrorAction SilentlyContinue

Add-Type -AssemblyName System.Drawing
Add-Type -ReferencedAssemblies @("System.Drawing") -TypeDefinition @"
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Drawing;
using System.Runtime.InteropServices;

public static class Pcsx2TraceNative {
  public delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam);

  [DllImport("user32.dll")] public static extern bool EnumWindows(EnumWindowsProc lpEnumFunc, IntPtr lParam);
  [DllImport("user32.dll")] public static extern bool EnumChildWindows(IntPtr hWnd, EnumWindowsProc lpEnumFunc, IntPtr lParam);
  [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint lpdwProcessId);
  [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr hWnd);
  [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr hWnd, out RECT lpRect);
  [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr hWnd, out RECT lpRect);
  [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr hWnd, uint msg, IntPtr wParam, IntPtr lParam);
  [DllImport("user32.dll")] public static extern uint MapVirtualKeyW(uint uCode, uint uMapType);
  [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr hwnd, IntPtr hdcBlt, uint nFlags);
  [DllImport("kernel32.dll")] public static extern IntPtr OpenProcess(uint dwDesiredAccess, bool bInheritHandle, uint dwProcessId);
  [DllImport("kernel32.dll")] public static extern bool ReadProcessMemory(IntPtr hProcess, IntPtr lpBaseAddress, byte[] lpBuffer, UIntPtr nSize, out UIntPtr lpNumberOfBytesRead);
  [DllImport("kernel32.dll")] public static extern bool WriteProcessMemory(IntPtr hProcess, IntPtr lpBaseAddress, byte[] lpBuffer, UIntPtr nSize, out UIntPtr lpNumberOfBytesWritten);
  [DllImport("kernel32.dll")] public static extern bool CloseHandle(IntPtr hObject);

  [StructLayout(LayoutKind.Sequential)]
  public struct RECT { public int Left, Top, Right, Bottom; }

  const uint PROCESS_VM_READ = 0x0010;
  const uint PROCESS_VM_WRITE = 0x0020;
  const uint PROCESS_VM_OPERATION = 0x0008;
  const uint PROCESS_QUERY_INFORMATION = 0x0400;
  const uint WM_KEYDOWN = 0x0100;
  const uint WM_KEYUP = 0x0101;

  public static IntPtr[] WindowsForPid(int pid) {
    List<IntPtr> wins = new List<IntPtr>();
    EnumWindows(delegate(IntPtr h, IntPtr p) {
      uint wpid;
      GetWindowThreadProcessId(h, out wpid);
      if (wpid == (uint)pid) {
        wins.Add(h);
        EnumChildWindows(h, delegate(IntPtr ch, IntPtr cp) {
          uint cpid;
          GetWindowThreadProcessId(ch, out cpid);
          if (cpid == (uint)pid) wins.Add(ch);
          return true;
        }, IntPtr.Zero);
      }
      return true;
    }, IntPtr.Zero);
    return wins.ToArray();
  }

  public static void Tap(IntPtr[] windows, int vk, int holdMs) {
    IntPtr w = new IntPtr(vk);
    uint scan = MapVirtualKeyW((uint)vk, 0);
    IntPtr down = new IntPtr((int)(1u | (scan << 16)));
    IntPtr up = new IntPtr((int)(1u | (scan << 16) | (1u << 30) | (1u << 31)));
    foreach (IntPtr h in windows) PostMessage(h, WM_KEYDOWN, w, down);
    System.Threading.Thread.Sleep(holdMs);
    foreach (IntPtr h in windows) PostMessage(h, WM_KEYUP, w, up);
  }

  public static byte[] ReadMemory(int pid, ulong address, int length) {
    IntPtr proc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, false, (uint)pid);
    if (proc == IntPtr.Zero) throw new Exception("OpenProcess failed");
    try {
      byte[] buf = new byte[length];
      UIntPtr read;
      if (!ReadProcessMemory(proc, new IntPtr(unchecked((long)address)), buf, new UIntPtr((uint)length), out read))
        throw new Exception("ReadProcessMemory failed");
      if ((ulong)read != (ulong)length) Array.Resize(ref buf, (int)(ulong)read);
      return buf;
    } finally {
      CloseHandle(proc);
    }
  }

  public static void WriteMemory(int pid, ulong address, byte[] data) {
    IntPtr proc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_WRITE | PROCESS_VM_OPERATION, false, (uint)pid);
    if (proc == IntPtr.Zero) throw new Exception("OpenProcess for write failed");
    try {
      UIntPtr written;
      if (!WriteProcessMemory(proc, new IntPtr(unchecked((long)address)), data, new UIntPtr((uint)data.Length), out written))
        throw new Exception("WriteProcessMemory failed");
      if ((ulong)written != (ulong)data.Length) throw new Exception("short WriteProcessMemory");
    } finally {
      CloseHandle(proc);
    }
  }

  public static int[] IndexOfAll(byte[] data, byte[] needle) {
    List<int> hits = new List<int>();
    if (needle.Length == 0 || data.Length < needle.Length) return hits.ToArray();
    for (int i = 0; i <= data.Length - needle.Length; ++i) {
      if (data[i] != needle[0]) continue;
      int j = 1;
      for (; j < needle.Length; ++j) if (data[i + j] != needle[j]) break;
      if (j == needle.Length) hits.Add(i);
    }
    return hits.ToArray();
  }

  public static int[] FindU32(byte[] data, uint value) {
    List<int> hits = new List<int>();
    byte b0 = (byte)(value & 0xff);
    byte b1 = (byte)((value >> 8) & 0xff);
    byte b2 = (byte)((value >> 16) & 0xff);
    byte b3 = (byte)((value >> 24) & 0xff);
    for (int i = 0; i <= data.Length - 4; i += 4) {
      if (data[i] == b0 && data[i + 1] == b1 && data[i + 2] == b2 && data[i + 3] == b3)
        hits.Add(i);
    }
    return hits.ToArray();
  }

  public static uint U32(byte[] data, int off) {
    if (off < 0 || off + 4 > data.Length) return 0;
    return BitConverter.ToUInt32(data, off);
  }

  public static float F32(byte[] data, int off) {
    if (off < 0 || off + 4 > data.Length) return 0.0f;
    return BitConverter.ToSingle(data, off);
  }

  public static string AsciiAt(byte[] data, int off, int maxLen) {
    if (off < 0 || off >= data.Length) return "";
    int end = off;
    int cap = Math.Min(data.Length, off + maxLen);
    while (end < cap && data[end] >= 0x20 && data[end] <= 0x7e) end++;
    return end > off ? System.Text.Encoding.ASCII.GetString(data, off, end - off) : "";
  }

  public static bool SaveWindow(IntPtr hwnd, string path) {
    RECT r;
    if (!GetWindowRect(hwnd, out r)) return false;
    int w = Math.Max(1, r.Right - r.Left);
    int h = Math.Max(1, r.Bottom - r.Top);
    using (Bitmap bmp = new Bitmap(w, h)) {
      using (Graphics g = Graphics.FromImage(bmp)) {
        IntPtr hdc = g.GetHdc();
        try {
          PrintWindow(hwnd, hdc, 2);
        } finally {
          g.ReleaseHdc(hdc);
        }
      }
      bmp.Save(path);
    }
    return true;
  }
}
"@

function Wait-ForLogBase {
  param([string]$Path, [int]$TimeoutSec)
  $deadline = (Get-Date).AddSeconds($TimeoutSec)
  while ((Get-Date) -lt $deadline) {
    if (Test-Path -LiteralPath $Path) {
      $text = Get-Content -LiteralPath $Path -Raw -ErrorAction SilentlyContinue
      if ($text -match "EE Main Memory\s+@\s+0x([0-9A-Fa-f]+)") {
        return [Convert]::ToUInt64($Matches[1], 16)
      }
    }
    Start-Sleep -Milliseconds 250
  }
  throw "Timed out waiting for PCSX2 EE base in log."
}

function Wait-ForWindows {
  param([int]$ProcessId, [int]$TimeoutSec)
  $deadline = (Get-Date).AddSeconds($TimeoutSec)
  while ((Get-Date) -lt $deadline) {
    $wins = [Pcsx2TraceNative]::WindowsForPid($ProcessId)
    if ($wins.Count -gt 0) { return $wins }
    Start-Sleep -Milliseconds 250
  }
  throw "Timed out waiting for PCSX2 windows."
}

function Tap-Key {
  param([IntPtr[]]$Windows, [int]$Vk, [int]$AfterMs = 300, [int]$HoldMs = 70)
  [Pcsx2TraceNative]::Tap($Windows, $Vk, $HoldMs)
  Start-Sleep -Milliseconds $AfterMs
}

function Capture-Pcsx2Snap {
  param([IntPtr[]]$Windows, [hashtable]$Vk, [string]$Label, [string]$SnapDir, [string]$OutDir)
  $windowDest = Join-Path $OutDir ($Label + ".printwindow.png")
  $windowOk = $false
  if ($Windows.Count -gt 0) {
    $windowOk = [Pcsx2TraceNative]::SaveWindow($Windows[0], $windowDest)
  }
  $shotStart = Get-Date
  Tap-Key $Windows $Vk.Screenshot 900
  Start-Sleep -Milliseconds 250
  $latestSnap = Get-ChildItem -LiteralPath $SnapDir -File -Filter "*.png" -ErrorAction SilentlyContinue |
      Where-Object { $_.LastWriteTime -gt $shotStart } |
      Sort-Object LastWriteTime -Descending |
      Select-Object -First 1
  if (-not $latestSnap) {
    return [ordered]@{
      native_snap = $null
      printwindow = $(if ($windowOk) { $windowDest } else { $null })
    }
  }
  $dest = Join-Path $OutDir ($Label + ".pcsx2snap.png")
  Copy-Item -LiteralPath $latestSnap.FullName -Destination $dest -Force
  return [ordered]@{
    native_snap = $dest
    printwindow = $(if ($windowOk) { $windowDest } else { $null })
  }
}

function U32-Hex([uint32]$v) {
  return ("0x{0:x8}" -f $v)
}

function Guest-Hex([int]$off) {
  return ("0x{0:x8}" -f $off)
}

function Decode-Matrix {
  param([byte[]]$Data, [int]$Offset)
  $rot = @()
  for ($r = 0; $r -lt 3; $r++) {
    $row = @()
    for ($c = 0; $c -lt 3; $c++) {
      $row += [Math]::Round([Pcsx2TraceNative]::F32($Data, $Offset + (($r * 3 + $c) * 4)), 6)
    }
    $rot += ,$row
  }
  $pos = @()
  for ($c = 0; $c -lt 3; $c++) {
    $pos += [Math]::Round([Pcsx2TraceNative]::F32($Data, $Offset + ((9 + $c) * 4)), 6)
  }
  [ordered]@{ rot = $rot; pos = $pos }
}

function String-Hits {
  param([byte[]]$Ram, [string]$Term)
  $needle = [Text.Encoding]::ASCII.GetBytes($Term)
  @([Pcsx2TraceNative]::IndexOfAll($Ram, $needle)) | Select-Object -First 32 | ForEach-Object { [int]$_ }
}

function Ascii-Window {
  param([byte[]]$Ram, [int]$Offset, [int]$Radius = 96)
  $start = [Math]::Max(0, $Offset - $Radius)
  $end = [Math]::Min($Ram.Length, $Offset + $Radius)
  $chars = New-Object System.Text.StringBuilder
  for ($i = $start; $i -lt $end; $i++) {
    $b = $Ram[$i]
    if ($b -ge 0x20 -and $b -le 0x7e) {
      [void]$chars.Append([char]$b)
    } else {
      [void]$chars.Append(".")
    }
  }
  [ordered]@{
    start_guest = (Guest-Hex $start)
    end_guest = (Guest-Hex $end)
    ascii = $chars.ToString()
  }
}

function Read-GuestRamSparse {
  param([int]$ProcessId, [UInt64]$Base, [int]$Length)
  $ram = New-Object byte[] $Length
  $chunk = 0x100000
  $readable = @()
  for ($off = 0; $off -lt $Length; $off += $chunk) {
    $len = [Math]::Min($chunk, $Length - $off)
    try {
      $part = [Pcsx2TraceNative]::ReadMemory($ProcessId, $Base + [UInt64]$off, $len)
      [Array]::Copy($part, 0, $ram, $off, $part.Length)
      $readable += [ordered]@{
        start_guest = (Guest-Hex $off)
        length = $part.Length
      }
    } catch {
    }
  }
  [ordered]@{ bytes = $ram; readable_ranges = $readable }
}

function U32-Bytes([uint32]$v) {
  return [BitConverter]::GetBytes($v)
}

function Join-ByteArrays {
  param([byte[][]]$Parts)
  $len = 0
  foreach ($p in $Parts) { $len += $p.Length }
  $out = New-Object byte[] $len
  $at = 0
  foreach ($p in $Parts) {
    [Array]::Copy($p, 0, $out, $at, $p.Length)
    $at += $p.Length
  }
  return $out
}

function Mips-J([uint32]$Target) {
  return [uint32](0x08000000 -bor (($Target -shr 2) -band 0x03ffffff))
}

function Install-DisplaySetupHook {
  param([int]$ProcessId, [UInt64]$Base)
  $hookBuffer = [uint32]0x01ffe000
  $hookCode = [uint32]0x01ffe100
  $setupEntry = [uint32]0x001470d8
  $setupResume = [uint32]0x001470e0

  # Records retail GuitarDisplayPanel entry setup calls:
  # +0x00 count, +0x04 a0 entry, +0x08 a1 proxy, +0x0c a2 guitar dir,
  # +0x10 a3 filter, +0x14 f12 loop length bits.
  $codeWords = @(
    "0x3c1a01ff",            # lui   k0,0x01ff
    "0x375ae000",            # ori   k0,k0,0xe000
    "0x8f5b0000",            # lw    k1,0(k0)
    "0x277b0001",            # addiu k1,k1,1
    "0xaf5b0000",            # sw    k1,0(k0)
    "0xaf440004",            # sw    a0,4(k0)
    "0xaf450008",            # sw    a1,8(k0)
    "0xaf46000c",            # sw    a2,0xc(k0)
    "0xaf470010",            # sw    a3,0x10(k0)
    "0x441b6000",            # mfc1  k1,f12
    "0xaf5b0014",            # sw    k1,0x14(k0)
    "0x27bdffa0",            # original: addiu sp,sp,-0x60
    "0x7fb10040",            # original EE: sq s1,0x40(sp)
    ("0x{0:x8}" -f (Mips-J $setupResume)),  # j 0x001470e0
    "0x00000000"             # nop
  )
  $codeBytes = Join-ByteArrays ($codeWords | ForEach-Object {
      U32-Bytes ([Convert]::ToUInt32($_.Substring(2), 16))
    })
  $patchBytes = Join-ByteArrays @((U32-Bytes (Mips-J $hookCode)), (U32-Bytes 0x00000000))
  $zero = New-Object byte[] 0x40
  [Pcsx2TraceNative]::WriteMemory($ProcessId, $Base + [UInt64]$hookBuffer, $zero)
  [Pcsx2TraceNative]::WriteMemory($ProcessId, $Base + [UInt64]$hookCode, $codeBytes)
  [Pcsx2TraceNative]::WriteMemory($ProcessId, $Base + [UInt64]$setupEntry, $patchBytes)
  [ordered]@{
    installed = $true
    method = "temporary EE RAM jump hook, process-local only"
    setup_entry_guest = (Guest-Hex $setupEntry)
    setup_resume_guest = (Guest-Hex $setupResume)
    hook_code_guest = (Guest-Hex $hookCode)
    hook_buffer_guest = (Guest-Hex $hookBuffer)
  }
}

function Read-DisplaySetupHookTrace {
  param([byte[]]$Ram)
  $hookBuffer = 0x01ffe000
  $count = [Pcsx2TraceNative]::U32($Ram, $hookBuffer + 0x00)
  $entry = [Pcsx2TraceNative]::U32($Ram, $hookBuffer + 0x04)
  $proxy = [Pcsx2TraceNative]::U32($Ram, $hookBuffer + 0x08)
  $guitar = [Pcsx2TraceNative]::U32($Ram, $hookBuffer + 0x0c)
  $filter = [Pcsx2TraceNative]::U32($Ram, $hookBuffer + 0x10)
  $loopBits = [Pcsx2TraceNative]::U32($Ram, $hookBuffer + 0x14)
  $loopBytes = [BitConverter]::GetBytes($loopBits)
  $loop = [BitConverter]::ToSingle($loopBytes, 0)
  $entryWords = @()
  if (Is-GuestPointer $entry $Ram.Length) {
    $entryWords = @(Pointer-Block $Ram ([int]$entry) 8)
  }
  [ordered]@{
    source = "temporary live PCSX2 EE hook at GuitarDisplayPanel entry setup"
    call_count = $count
    entry_guest = (U32-Hex $entry)
    proxy_arg = (Object-Probe $Ram $proxy)
    guitar_dir_arg = (Object-Probe $Ram $guitar)
    filter_arg = (Object-Probe $Ram $filter)
    loop_length = [Math]::Round($loop, 6)
    entry_words_after_setup = $entryWords
  }
}

function Pointer-Block {
  param([byte[]]$Ram, [int]$Base, [int]$Words = 32)
  $items = @()
  for ($i = 0; $i -lt $Words; $i++) {
    $addr = $Base + ($i * 4)
    if ($addr -lt 0 -or $addr + 4 -gt $Ram.Length) { break }
    $u = [Pcsx2TraceNative]::U32($Ram, $addr)
    $ascii = ""
    if ($u -lt [uint32]$Ram.Length) {
      $ascii = [Pcsx2TraceNative]::AsciiAt($Ram, [int]$u, 64)
    }
    $items += [ordered]@{
      guest = (Guest-Hex $addr)
      u32 = (U32-Hex $u)
      deref_ascii = $ascii
    }
  }
  $items
}

function Float-Block {
  param([byte[]]$Ram, [int]$Base, [int]$Words = 32)
  $items = @()
  for ($i = 0; $i -lt $Words; $i++) {
    $addr = $Base + ($i * 4)
    if ($addr -lt 0 -or $addr + 4 -gt $Ram.Length) { break }
    $u = [Pcsx2TraceNative]::U32($Ram, $addr)
    $f = [Pcsx2TraceNative]::F32($Ram, $addr)
    $ascii = ""
    if (Is-GuestPointer $u $Ram.Length) {
      $ascii = [Pcsx2TraceNative]::AsciiAt($Ram, [int]$u, 64)
    }
    $items += [ordered]@{
      guest = (Guest-Hex $addr)
      offset_from_base = ("0x{0:x}" -f ($addr - $Base))
      u32 = (U32-Hex $u)
      f32 = $(if ((-not [float]::IsNaN($f)) -and
                  (-not [float]::IsInfinity($f)) -and
                  [Math]::Abs($f) -lt 1000000.0) {
          [Math]::Round($f, 6)
        } else { $null })
      deref_ascii = $ascii
    }
  }
  $items
}

function Pointer-Fields {
  param([byte[]]$Ram, [int]$Base, [int]$Bytes = 0x240, [int]$Limit = 48)
  $items = @()
  if (-not (Is-GuestPointer ([uint32]$Base) $Ram.Length)) { return $items }
  for ($off = 0; $off -le $Bytes - 4; $off += 4) {
    $u = [Pcsx2TraceNative]::U32($Ram, $Base + $off)
    if (-not (Is-GuestPointer $u $Ram.Length)) { continue }
    $items += [ordered]@{
      offset = ("0x{0:x}" -f $off)
      guest = (Guest-Hex ($Base + $off))
      u32 = (U32-Hex $u)
      ascii_window = (Ascii-Window $Ram ([int]$u) 64)
      first_words = @(Pointer-Block $Ram ([int]$u) 8)
      matrix_candidates = @(Matrix-CandidatesNear $Ram $u 0x180 6)
    }
    if ($items.Count -ge $Limit) { break }
  }
  $items
}

function Trace-ShowGuitarCode {
  param([byte[]]$Ram)
  $sites = @(
    [ordered]@{
      guest = 0x00147c08
      expected = "0x3c050040"
      meaning = "show_guitar handler: load high half of show_guitar symbol literal"
    },
    [ordered]@{
      guest = 0x00147d0c
      expected = "0x02c0282d"
      meaning = "show_guitar handler: pass player index to display helper"
    },
    [ordered]@{
      guest = 0x00147d10
      expected = "0x0280302d"
      meaning = "show_guitar handler: pass guitar symbol to display helper"
    },
    [ordered]@{
      guest = 0x00147d14
      expected = "0x0240382d"
      meaning = "show_guitar handler: pass skin symbol to display helper"
    },
    [ordered]@{
      guest = 0x00147d18
      expected = "0x0220402d"
      meaning = "show_guitar handler: pass resolved proxy object"
    },
    [ordered]@{
      guest = 0x00147d1c
      expected = "0x0c051e6e"
      meaning = "show_guitar handler: call GuitarDisplayPanel display helper at 0x001479b8"
    },
    [ordered]@{
      guest = 0x00147d20
      expected = "0x0040482d"
      meaning = "show_guitar handler delay slot: pass resolved filter object"
    },
    [ordered]@{
      guest = 0x00147aac
      expected = "0x3c014370"
      meaning = "display helper: load 240.0f loop length high half"
    },
    [ordered]@{
      guest = 0x00147ab0
      expected = "0x44816000"
      meaning = "display helper: move 240.0f into f12 for setup call"
    },
    [ordered]@{
      guest = 0x00147ac0
      expected = "0x0c051c36"
      meaning = "display helper: call display-entry setup at 0x001470d8"
    },
    [ordered]@{
      guest = 0x00147100
      expected = "0xae27000c"
      meaning = "entry setup: store filter object at entry+0x0c"
    },
    [ordered]@{
      guest = 0x00147108
      expected = "0xe62c0010"
      meaning = "entry setup: store f12 loop length at entry+0x10"
    },
    [ordered]@{
      guest = 0x00147110
      expected = "0x44806000"
      meaning = "entry setup: move 0.0f into f12 before filter vtable call"
    },
    [ordered]@{
      guest = 0x00147118
      expected = "0x3c013f80"
      meaning = "entry setup: load 1.0f high half for paired filter call argument"
    },
    [ordered]@{
      guest = 0x00147128
      expected = "0x0060f809"
      meaning = "entry setup: call filter vtable with f12=0.0f/f13=1.0f"
    }
  )
  $items = @()
  foreach ($site in $sites) {
    $addr = [int]$site.guest
    $word = [Pcsx2TraceNative]::U32($Ram, $addr)
    $expected = [Convert]::ToUInt32($site.expected.Substring(2), 16)
    $items += [ordered]@{
      guest = (Guest-Hex $addr)
      expected_u32 = (U32-Hex $expected)
      live_u32 = (U32-Hex $word)
      matches_expected = ($word -eq $expected)
      meaning = $site.meaning
    }
  }
  [ordered]@{
    source = "live PCSX2 EE RAM, selected SLUS_214.47 GuitarDisplayPanel sites"
    conclusion = "Retail show_guitar uses the script proxy/filter, initializes the filter at frame 0.0, stores a 240.0-frame loop length, then advances the filter at runtime."
    selected_sites = $items
  }
}

function Is-GuestPointer {
  param([uint32]$Value, [int]$RamLength)
  return ($Value -ge 0x00100000 -and $Value -lt [uint32]$RamLength -and
          (($Value -band 0x3) -eq 0))
}

function Matrix-CandidatesNear {
  param([byte[]]$Ram, [uint32]$Base, [int]$Radius = 0x260, [int]$Limit = 16)
  function Test-FiniteFloat([float]$Value) {
    return (-not [float]::IsNaN($Value)) -and
           (-not [float]::IsInfinity($Value))
  }
  $out = @()
  if (-not (Is-GuestPointer $Base $Ram.Length)) { return $out }
  $start = [Math]::Max(0, [int]$Base - 0x20)
  $end = [Math]::Min($Ram.Length - 48, [int]$Base + $Radius)
  for ($off = $start; $off -le $end; $off += 4) {
    $rowsOk = $true
    for ($r = 0; $r -lt 3; $r++) {
      $len2 = 0.0
      for ($c = 0; $c -lt 3; $c++) {
        $f = [Pcsx2TraceNative]::F32($Ram, $off + (($r * 3 + $c) * 4))
        if (-not (Test-FiniteFloat $f) -or [Math]::Abs($f) -gt 4.0) {
          $rowsOk = $false
          break
        }
        $len2 += [double]$f * [double]$f
      }
      if (-not $rowsOk) { break }
      if ($len2 -lt 0.05 -or $len2 -gt 16.0) {
        $rowsOk = $false
        break
      }
    }
    if (-not $rowsOk) { continue }
    $pos = @(
      [Pcsx2TraceNative]::F32($Ram, $off + 36),
      [Pcsx2TraceNative]::F32($Ram, $off + 40),
      [Pcsx2TraceNative]::F32($Ram, $off + 44)
    )
    $posOk = $true
    foreach ($f in $pos) {
      if (-not (Test-FiniteFloat $f) -or [Math]::Abs($f) -gt 20000.0) {
        $posOk = $false
        break
      }
    }
    if (-not $posOk) { continue }
    $out += [ordered]@{
      guest = (Guest-Hex $off)
      offset_from_base = ("0x{0:x}" -f ($off - [int]$Base))
      xfm = (Decode-Matrix $Ram $off)
    }
    if ($out.Count -ge $Limit) { break }
  }
  return $out
}

function Runtime-Transform-CandidatesNear {
  param([byte[]]$Ram, [uint32]$Base, [int]$Radius = 0x400, [int]$Limit = 24)
  $out = @()
  if (-not (Is-GuestPointer $Base $Ram.Length)) { return $out }
  $start = [Math]::Max(0, [int]$Base - 0x20)
  $end = [Math]::Min($Ram.Length - 0x40, [int]$Base + $Radius)
  for ($off = $start; $off -le $end; $off += 4) {
    $rot = @()
    $valid = $true
    for ($row = 0; $row -lt 3; $row++) {
      $vec = @()
      $len2 = 0.0
      for ($col = 0; $col -lt 3; $col++) {
        $value = [Pcsx2TraceNative]::F32(
            $Ram, $off + ($row * 0x10) + ($col * 4))
        if ([float]::IsNaN($value) -or [float]::IsInfinity($value) -or
            [Math]::Abs($value) -gt 4.0) {
          $valid = $false
          break
        }
        $vec += [Math]::Round($value, 6)
        $len2 += [double]$value * [double]$value
      }
      if (-not $valid -or $len2 -lt 0.05 -or $len2 -gt 16.0) {
        $valid = $false
        break
      }
      $rot += ,$vec
    }
    if (-not $valid) { continue }
    $pos = @()
    for ($col = 0; $col -lt 3; $col++) {
      $value = [Pcsx2TraceNative]::F32($Ram, $off + 0x30 + ($col * 4))
      if ([float]::IsNaN($value) -or [float]::IsInfinity($value) -or
          [Math]::Abs($value) -gt 20000.0) {
        $valid = $false
        break
      }
      $pos += [Math]::Round($value, 6)
    }
    if (-not $valid) { continue }
    $out += [ordered]@{
      guest = (Guest-Hex $off)
      offset_from_base = ("0x{0:x}" -f ($off - [int]$Base))
      xfm = [ordered]@{ rot = $rot; pos = $pos }
    }
    if ($out.Count -ge $Limit) { break }
  }
  return $out
}

function Decode-Runtime-Transform {
  param([byte[]]$Ram, [uint32]$Guest)
  if ($Guest -gt ($Ram.Length - 0x40)) { return $null }
  $rot = @()
  for ($row = 0; $row -lt 3; $row++) {
    $rot += ,@(
      [Math]::Round([Pcsx2TraceNative]::F32($Ram, [int]$Guest + ($row * 0x10)), 6),
      [Math]::Round([Pcsx2TraceNative]::F32($Ram, [int]$Guest + ($row * 0x10) + 4), 6),
      [Math]::Round([Pcsx2TraceNative]::F32($Ram, [int]$Guest + ($row * 0x10) + 8), 6)
    )
  }
  return [ordered]@{
    guest = (Guest-Hex $Guest)
    rot = $rot
    pos = @(
      [Math]::Round([Pcsx2TraceNative]::F32($Ram, [int]$Guest + 0x30), 6),
      [Math]::Round([Pcsx2TraceNative]::F32($Ram, [int]$Guest + 0x34), 6),
      [Math]::Round([Pcsx2TraceNative]::F32($Ram, [int]$Guest + 0x38), 6)
    )
  }
}

function Filter-Probe {
  param([byte[]]$Ram, [uint32]$Ptr)
  if (-not (Is-GuestPointer $Ptr $Ram.Length)) {
    return [ordered]@{ guest = (U32-Hex $Ptr); valid_pointer = $false }
  }
  $frame = [Pcsx2TraceNative]::F32($Ram, [int]$Ptr + 4)
  $rate = [Pcsx2TraceNative]::F32($Ram, [int]$Ptr + 8)
  [ordered]@{
    guest = (U32-Hex $Ptr)
    valid_pointer = $true
    probable_runtime_frame_at_0x04 = $(if ((-not [float]::IsNaN($frame)) -and
        (-not [float]::IsInfinity($frame))) { [Math]::Round($frame, 6) } else { $null })
    probable_rate_at_0x08 = $(if ((-not [float]::IsNaN($rate)) -and
        (-not [float]::IsInfinity($rate))) { [Math]::Round($rate, 6) } else { $null })
    first_words = @(Pointer-Block $Ram ([int]$Ptr) 32)
    first_floats = @(Float-Block $Ram ([int]$Ptr) 32)
    pointer_fields = @(Pointer-Fields $Ram ([int]$Ptr) 0x180 24)
    matrix_candidates = @(Matrix-CandidatesNear $Ram $Ptr 0x500 32)
  }
}

function Runtime-Attachment-Probe {
  param([byte[]]$Ram, [uint32]$Proxy, [uint32]$Filter)
  $child = if (Is-GuestPointer $Proxy $Ram.Length) {
    [Pcsx2TraceNative]::U32($Ram, [int]$Proxy + 0x144)
  } else { [uint32]0 }
  [ordered]@{
    source = "live PCSX2 EE RAM after settled Career guitar select"
    meaning = "Retail entry stores guitar.pxy, but update code attaches through proxy+0x144; this probes that runtime child plus the current filter frame."
    proxy = (Object-Probe $Ram $Proxy)
    proxy_runtime_child_0x144 = (U32-Hex $child)
    proxy_near_floats = $(if (Is-GuestPointer $Proxy $Ram.Length) {
        @(Float-Block $Ram ([int]$Proxy) 96)
      } else { @() })
    child_probe = $(if (Is-GuestPointer $child $Ram.Length) {
        [ordered]@{
          ascii_window = (Ascii-Window $Ram ([int]$child) 160)
          first_words = @(Pointer-Block $Ram ([int]$child) 96)
          first_floats = @(Float-Block $Ram ([int]$child) 96)
          pointer_fields = @(Pointer-Fields $Ram ([int]$child) 0x420 64)
          matrix_candidates_wide = @(Matrix-CandidatesNear $Ram $child 0x3000 80)
        }
      } else { $null })
    filter_probe = (Filter-Probe $Ram $Filter)
  }
}

function Object-Probe {
  param([byte[]]$Ram, [uint32]$Ptr)
  if (-not (Is-GuestPointer $Ptr $Ram.Length)) {
    return [ordered]@{
      guest = (U32-Hex $Ptr)
      valid_pointer = $false
    }
  }
  $field144 = [Pcsx2TraceNative]::U32($Ram, [int]$Ptr + 0x144)
  $words = @()
  for ($off = 0; $off -le 0x170; $off += 4) {
    $u = [Pcsx2TraceNative]::U32($Ram, [int]$Ptr + $off)
    $ascii = ""
    if (Is-GuestPointer $u $Ram.Length) {
      $ascii = [Pcsx2TraceNative]::AsciiAt($Ram, [int]$u, 80)
    }
    if ($ascii -or $off -eq 0 -or $off -eq 8 -or $off -eq 0x144 -or
        $off -eq 0x148 -or $off -eq 0x14c) {
      $words += [ordered]@{
        offset = ("0x{0:x}" -f $off)
        u32 = (U32-Hex $u)
        deref_ascii = $ascii
      }
    }
  }
  [ordered]@{
    guest = (U32-Hex $Ptr)
    valid_pointer = $true
    ascii_window = (Ascii-Window $Ram ([int]$Ptr) 80)
    selected_words = $words
    field_0x144 = (U32-Hex $field144)
    field_0x144_probe = $(if (Is-GuestPointer $field144 $Ram.Length) {
        [ordered]@{
          ascii_window = (Ascii-Window $Ram ([int]$field144) 80)
          selected_words = @(Pointer-Block $Ram ([int]$field144) 24)
          matrix_candidates = @(Matrix-CandidatesNear $Ram $field144 0x260)
        }
      } else { $null })
    matrix_candidates = @(Matrix-CandidatesNear $Ram $Ptr 0x260)
  }
}

function Trace-GuitarDisplayEntries {
  param([byte[]]$Ram)
  $entries = @()
  $loopWord = [BitConverter]::ToUInt32([BitConverter]::GetBytes([single]240.0), 0)
  $loopHits = @([Pcsx2TraceNative]::FindU32($Ram, $loopWord))
  foreach ($hit in $loopHits) {
    $entry = [int]$hit - 0x10
    if ($entry -lt 0 -or $entry + 0x18 -gt $Ram.Length) { continue }
    $proxy = [Pcsx2TraceNative]::U32($Ram, $entry + 0x00)
    $guitar = [Pcsx2TraceNative]::U32($Ram, $entry + 0x04)
    $loaded = [Pcsx2TraceNative]::U32($Ram, $entry + 0x08)
    $filter = [Pcsx2TraceNative]::U32($Ram, $entry + 0x0c)
    $loop = [Pcsx2TraceNative]::F32($Ram, $entry + 0x10)
    $env = [Pcsx2TraceNative]::U32($Ram, $entry + 0x14)
    if (-not (Is-GuestPointer $proxy $Ram.Length) -or
        -not (Is-GuestPointer $guitar $Ram.Length) -or
        -not (Is-GuestPointer $filter $Ram.Length)) {
      continue
    }
    if ($loaded -gt 4) { continue }
    $refs = @([Pcsx2TraceNative]::FindU32($Ram, [uint32]$entry) |
              Select-Object -First 16 |
              ForEach-Object { Guest-Hex $_ })
    $entries += [ordered]@{
      entry_guest = (Guest-Hex $entry)
      loop_hit_guest = (Guest-Hex $hit)
      entry_refs = $refs
      proxy = (Object-Probe $Ram $proxy)
      guitar_dir = (Object-Probe $Ram $guitar)
      loaded_flag = $loaded
      filter = (Object-Probe $Ram $filter)
      filter_runtime = (Filter-Probe $Ram $filter)
      loop_length = [Math]::Round($loop, 6)
      env = (Object-Probe $Ram $env)
      runtime_attachment = (Runtime-Attachment-Probe $Ram $proxy $filter)
      raw_words = @(Pointer-Block $Ram $entry 6)
    }
    if ($entries.Count -ge 8) { break }
  }
  [ordered]@{
    source = "live PCSX2 EE RAM after reaching settled guitar select"
    search = "0x43700000 loop-length word at display-entry+0x10, then proxy/guitar/filter pointer validation"
    candidates = $entries
  }
}

$existing = @(Get-Process pcsx2* -ErrorAction SilentlyContinue)
if ($existing.Count -gt 0) {
  throw "PCSX2 is already running; refusing to clobber an active instance."
}

$argList = if ($FreshBoot) {
  "-logfile `"$log`" -- `"$Iso`""
} else {
  "-logfile `"$log`" -state $StateSlot -- `"$Iso`""
}
$pcsxIniPath = Join-Path (Split-Path -Parent $Pcsx2) "inis\PCSX2.ini"
$pcsxIniOriginal = $null
if ($DisarmTogglePauseHotkey) {
  $pcsxIniOriginal = [IO.File]::ReadAllBytes($pcsxIniPath)
  $pcsxIniText = [Text.Encoding]::UTF8.GetString($pcsxIniOriginal)
  $pcsxIniText = [Text.RegularExpressions.Regex]::Replace(
      $pcsxIniText, "(?m)^TogglePause\s*=.*$",
      "TogglePause = Keyboard/F12")
  [IO.File]::WriteAllText(
      $pcsxIniPath, $pcsxIniText, [Text.UTF8Encoding]::new($false))
}
$proc = Start-Process -FilePath $Pcsx2 -ArgumentList $argList -WorkingDirectory (Split-Path -Parent $Pcsx2) -WindowStyle $OracleWindowStyle -PassThru

try {
  $eeBase = Wait-ForLogBase -Path $log -TimeoutSec 30
  $wins = Wait-ForWindows -ProcessId $proc.Id -TimeoutSec 30
  Start-Sleep -Seconds 4
  $setupHook = $null
  if (-not $FreshBoot -and -not $SkipHook) {
    $setupHook = Install-DisplaySetupHook -ProcessId $proc.Id -Base $eeBase
  }

  $vk = @{
    Cross = 0x4c
    Triangle = 0x4f
    Start = 0x20
    Down = 0x48
    Up = 0x59
    Screenshot = 0x77
  }
  $snapDir = Join-Path (Split-Path -Parent $Pcsx2) "snaps"
  $stepProofs = @()

  if ($TraceCurrentScreen) {
    Start-Sleep -Seconds 5
    for ($i = 1; $i -le $CurrentScreenCrossCount; $i++) {
      Tap-Key $wins $vk.Cross 3500
      if ($CaptureSteps) {
        $stepProofs += [ordered]@{
          step = "current_screen_cross_$i"
          proof = (Capture-Pcsx2Snap $wins $vk "current_screen_cross_$i" $snapDir $OutDir)
        }
      }
    }
    if ($DeclineSaveAfterCurrentCrosses) {
      Tap-Key $wins $vk.Down 800
      Tap-Key $wins $vk.Cross 5000
    }
  } else {
  if ($FreshBoot) {
    # Fresh boot route: wait through logos/title, press Start to reach the stock
    # main menu, then enter Career so guitar select is available.
    Start-Sleep -Seconds 18
    if ($CaptureSteps) { $stepProofs += [ordered]@{ step = "fresh_boot_wait"; proof = (Capture-Pcsx2Snap $wins $vk "fresh_boot_wait" $snapDir $OutDir) } }
    Tap-Key $wins $vk.Down 350
    if ($CaptureSteps) { $stepProofs += [ordered]@{ step = "save_prompt_down"; proof = (Capture-Pcsx2Snap $wins $vk "save_prompt_down" $snapDir $OutDir) } }
    Tap-Key $wins $vk.Cross 2200
    if ($CaptureSteps) { $stepProofs += [ordered]@{ step = "save_prompt_confirm"; proof = (Capture-Pcsx2Snap $wins $vk "save_prompt_confirm" $snapDir $OutDir) } }
    Tap-Key $wins $vk.Up 350
    if ($CaptureSteps) { $stepProofs += [ordered]@{ step = "autosave_caution_yes"; proof = (Capture-Pcsx2Snap $wins $vk "autosave_caution_yes" $snapDir $OutDir) } }
    Tap-Key $wins $vk.Cross 1800
    if ($CaptureSteps) { $stepProofs += [ordered]@{ step = "autosave_caution_confirm"; proof = (Capture-Pcsx2Snap $wins $vk "autosave_caution_confirm" $snapDir $OutDir) } }
    Tap-Key $wins $vk.Cross 2200
    if ($CaptureSteps) { $stepProofs += [ordered]@{ step = "autosave_disable_confirm"; proof = (Capture-Pcsx2Snap $wins $vk "autosave_disable_confirm" $snapDir $OutDir) } }
    for ($i = 1; $i -le 5; $i++) {
      Tap-Key $wins $vk.Start 1800
      if ($CaptureSteps) { $stepProofs += [ordered]@{ step = "fresh_start_$i"; proof = (Capture-Pcsx2Snap $wins $vk "fresh_start_$i" $snapDir $OutDir) } }
    }
    Tap-Key $wins $vk.Cross 1800
    if ($CaptureSteps) { $stepProofs += [ordered]@{ step = "controller_help_continue"; proof = (Capture-Pcsx2Snap $wins $vk "controller_help_continue" $snapDir $OutDir) } }
    Tap-Key $wins $vk.Cross 1800
    if ($CaptureSteps) { $stepProofs += [ordered]@{ step = "title_continue_to_main"; proof = (Capture-Pcsx2Snap $wins $vk "title_continue_to_main" $snapDir $OutDir) } }
    Tap-Key $wins $vk.Up 450
    if ($CaptureSteps) { $stepProofs += [ordered]@{ step = "main_after_up"; proof = (Capture-Pcsx2Snap $wins $vk "main_after_up" $snapDir $OutDir) } }
    if (-not $SkipHook) {
      $setupHook = Install-DisplaySetupHook -ProcessId $proc.Id -Base $eeBase
    }
  } else {
    # Legacy route from the retry savestate. Kept for old comparison captures;
    # guitar parity tracing should prefer -FreshBoot.
    Tap-Key $wins $vk.Cross 1500
    if ($CaptureSteps) { $stepProofs += [ordered]@{ step = "cross_retry"; proof = (Capture-Pcsx2Snap $wins $vk "cross_retry" $snapDir $OutDir) } }
    Tap-Key $wins $vk.Start 900
    if ($CaptureSteps) { $stepProofs += [ordered]@{ step = "start_pause"; proof = (Capture-Pcsx2Snap $wins $vk "start_pause" $snapDir $OutDir) } }
    1..4 | ForEach-Object { Tap-Key $wins $vk.Down 240 }
    Tap-Key $wins $vk.Cross 700
    Tap-Key $wins $vk.Cross 3000
    if ($CaptureSteps) { $stepProofs += [ordered]@{ step = "quit_confirmed"; proof = (Capture-Pcsx2Snap $wins $vk "quit_confirmed" $snapDir $OutDir) } }
    Start-Sleep -Seconds 3
    if ($CaptureSteps) { $stepProofs += [ordered]@{ step = "setlist_ready"; proof = (Capture-Pcsx2Snap $wins $vk "setlist_ready" $snapDir $OutDir) } }
    Tap-Key $wins $vk.Triangle 1500
    if ($CaptureSteps) { $stepProofs += [ordered]@{ step = "triangle_to_main"; proof = (Capture-Pcsx2Snap $wins $vk "triangle_to_main" $snapDir $OutDir) } }
    Tap-Key $wins $vk.Up 450
    if ($CaptureSteps) { $stepProofs += [ordered]@{ step = "main_after_up"; proof = (Capture-Pcsx2Snap $wins $vk "main_after_up" $snapDir $OutDir) } }
  }
  Tap-Key $wins $vk.Cross 2200
  if ($CaptureSteps) { $stepProofs += [ordered]@{ step = "career_cross"; proof = (Capture-Pcsx2Snap $wins $vk "career_cross" $snapDir $OutDir) } }
  if ($CareerTrace) {
    Start-Sleep -Seconds 3
  } elseif ($PostCareerCrossCount -ge 0) {
    Tap-Key $wins $vk.Down 500
    if ($CaptureSteps) { $stepProofs += [ordered]@{ step = "post_career_down"; proof = (Capture-Pcsx2Snap $wins $vk "post_career_down" $snapDir $OutDir) } }
    for ($i = 1; $i -le $PostCareerCrossCount; $i++) {
      $wait = if ($i -eq $PostCareerCrossCount) { 2800 } else { 1200 }
      Tap-Key $wins $vk.Cross $wait
      if ($CaptureSteps) {
        $stepProofs += [ordered]@{ step = "post_career_cross_$i"; proof = (Capture-Pcsx2Snap $wins $vk "post_career_cross_$i" $snapDir $OutDir) }
      }
    }
  } else {
    Tap-Key $wins $vk.Down 250
    1..8 | ForEach-Object { Tap-Key $wins $vk.Cross 190 }
    Tap-Key $wins $vk.Start 900
    if ($CaptureSteps) { $stepProofs += [ordered]@{ step = "accept_name_start"; proof = (Capture-Pcsx2Snap $wins $vk "accept_name_start" $snapDir $OutDir) } }
    for ($i = 1; $i -le $SetupCrossCount; $i++) {
      $wait = if ($i -eq $SetupCrossCount) { 4000 } else { 1000 }
      Tap-Key $wins $vk.Cross $wait
      if ($CaptureSteps) {
        $stepProofs += [ordered]@{ step = "setup_cross_$i"; proof = (Capture-Pcsx2Snap $wins $vk "setup_cross_$i" $snapDir $OutDir) }
      }
    }
  }
  }
  Start-Sleep -Seconds 3

  $freshSnapProof = $false
  if (-not $NoProofCapture) {
    $finalShotStart = Get-Date
    Tap-Key $wins $vk.Screenshot 1000
    $latestSnap = Get-ChildItem -LiteralPath $snapDir -File -Filter "*.png" -ErrorAction SilentlyContinue |
        Where-Object { $_.LastWriteTime -gt $finalShotStart } |
        Sort-Object LastWriteTime -Descending |
        Select-Object -First 1
    if ($latestSnap) {
      Copy-Item -LiteralPath $latestSnap.FullName -Destination $snapProofPng -Force
      $freshSnapProof = $true
    }
    $topWindow = $wins[0]
    [Pcsx2TraceNative]::SaveWindow($topWindow, $proofPng) | Out-Null
  }

  $ramRead = Read-GuestRamSparse -ProcessId $proc.Id -Base $eeBase -Length 0x08f00000
  if ($ramRead.readable_ranges.Count -eq 0) {
    throw "No PCSX2 EE RAM chunks were readable; refusing to write a false trace."
  }
  $ram = [byte[]]$ramRead.bytes
  if ($CareerTrace) {
    $careerObjects = @()
    foreach ($term in @(
        "career.view",
        "cm_status.view",
        "cm_headers.grp",
        "cm_details.grp",
        "cm_buttons.view",
        "cm_featuring.lbl",
        "cm_band.lbl",
        "cm_char.lbl",
        "cm_playing.lbl",
        "cm_guitar.lbl",
        "cm_guitar_skin.lbl",
        "cm_career_title.lbl",
        "cm_career.lbl",
        "cm_cash_title.lbl",
        "cm_cash.lbl",
        "cm_letsrock.btn",
        "cm_hero.btn",
        "cm_guitar.btn",
        "cm_store.btn")) {
      $hitItems = @()
      $allRefs = @()
      foreach ($hit in @(String-Hits $ram $term | Select-Object -First 8)) {
        $refItems = @()
        foreach ($ref in @([Pcsx2TraceNative]::FindU32(
              $ram, [uint32]$hit) | Select-Object -First 12)) {
          $allRefs += [uint32]$ref
          $objectBase = [Math]::Max(0, [int]$ref - 0x18)
          $nearPointers = @()
          for ($nearOff = -0x40; $nearOff -le 0x40; $nearOff += 4) {
            $field = [int]$ref + $nearOff
            if ($field -lt 0 -or $field + 4 -gt $ram.Length) { continue }
            $ptr = [Pcsx2TraceNative]::U32($ram, $field)
            if (-not (Is-GuestPointer $ptr $ram.Length) -or
                $ptr -eq [uint32]$hit) {
              continue
            }
            $pointerMatrices = @(
                Runtime-Transform-CandidatesNear $ram $ptr 0x400 16)
            if ($pointerMatrices.Count -eq 0) { continue }
            $nearPointers += [ordered]@{
              field_offset_from_ref = ("0x{0:x}" -f $nearOff)
              field_guest = (Guest-Hex $field)
              pointer_guest = (U32-Hex $ptr)
              matrices = $pointerMatrices
            }
          }
          $refItems += [ordered]@{
            ref_guest = (Guest-Hex $ref)
            object_base_if_name_at_0x18 = (Guest-Hex $objectBase)
            matrices_wide = @(Runtime-Transform-CandidatesNear $ram (
                [Math]::Max(0, [int]$ref - 0x400)) 0x800 24)
            nearby_pointer_matrices = $nearPointers
          }
        }
        $hitItems += [ordered]@{
          string_guest = (Guest-Hex $hit)
          references = $refItems
        }
      }
      $liveComponent = $null
      if ($term.EndsWith(".lbl") -and $allRefs.Count -gt 0) {
        # GH2 UILabel's virtual Hmx::Object name field is at +0x174. The
        # highest live reference is the instantiated Career component; the
        # lower references belong to serialized/panel lookup structures.
        $liveNameRef = @($allRefs | Sort-Object -Descending |
            Where-Object { $_ -ge 0x00b00000 } | Select-Object -First 1)
        if ($liveNameRef.Count -gt 0) {
          $componentBase = [uint32]($liveNameRef[0] - 0x174)
          $componentPointers = @()
          for ($fieldOff = 0; $fieldOff -le 0x170; $fieldOff += 4) {
            $fieldGuest = [uint32]($componentBase + $fieldOff)
            $fieldPtr = [Pcsx2TraceNative]::U32($ram, [int]$fieldGuest)
            if ($fieldPtr -lt 0x00b00000 -or
                -not (Is-GuestPointer $fieldPtr $ram.Length)) {
              continue
            }
            $componentPointers += [ordered]@{
              field_offset = ("0x{0:x}" -f $fieldOff)
              field_guest = (Guest-Hex $fieldGuest)
              pointer_guest = (U32-Hex $fieldPtr)
              target_transform_candidates = @(
                  Runtime-Transform-CandidatesNear $ram $fieldPtr 0x200 12)
            }
          }
          $layoutWords = @()
          for ($fieldOff = 0; $fieldOff -le 0x178; $fieldOff += 4) {
            $fieldGuest = [uint32]($componentBase + $fieldOff)
            $layoutWords += [ordered]@{
              field_offset = ("0x{0:x}" -f $fieldOff)
              u32 = (U32-Hex (
                  [Pcsx2TraceNative]::U32($ram, [int]$fieldGuest)))
              f32 = [Math]::Round(
                  [Pcsx2TraceNative]::F32($ram, [int]$fieldGuest), 6)
            }
          }
          $vectorCandidates = @()
          for ($fieldOff = 0; $fieldOff -le 0x170; $fieldOff += 4) {
            $startPtr = [Pcsx2TraceNative]::U32(
                $ram, [int]$componentBase + $fieldOff)
            $finishPtr = [Pcsx2TraceNative]::U32(
                $ram, [int]$componentBase + $fieldOff + 4)
            $endPtr = [Pcsx2TraceNative]::U32(
                $ram, [int]$componentBase + $fieldOff + 8)
            if ((Is-GuestPointer $startPtr $ram.Length) -and
                (Is-GuestPointer $finishPtr $ram.Length) -and
                (Is-GuestPointer $endPtr $ram.Length) -and
                $startPtr -le $finishPtr -and $finishPtr -le $endPtr -and
                ($endPtr - $startPtr) -le 0x10000) {
              $vectorCandidates += [ordered]@{
                field_offset = ("0x{0:x}" -f $fieldOff)
                start = (U32-Hex $startPtr)
                finish = (U32-Hex $finishPtr)
                end = (U32-Hex $endPtr)
                used_bytes = [uint32]($finishPtr - $startPtr)
                capacity_bytes = [uint32]($endPtr - $startPtr)
              }
            }
          }
          $liveComponent = [ordered]@{
            name_reference = (Guest-Hex $liveNameRef[0])
            rnd_text_base = (Guest-Hex $componentBase)
            rnd_text_local = (Decode-Runtime-Transform $ram ([uint32]($componentBase + 0x20)))
            rnd_text_world = (Decode-Runtime-Transform $ram ([uint32]($componentBase + 0x60)))
            rnd_text_size = [Math]::Round(
                [Pcsx2TraceNative]::F32($ram, [int]$componentBase + 0x118), 6)
            rnd_text_alignment = (U32-Hex (
                [Pcsx2TraceNative]::U32($ram, [int]$componentBase + 0x130)))
            rnd_text_leading = [Math]::Round(
                [Pcsx2TraceNative]::F32($ram, [int]$componentBase + 0x134), 6)
            rnd_text_kerning = [Math]::Round(
                [Pcsx2TraceNative]::F32($ram, [int]$componentBase + 0x138), 6)
            ui_label_fit = (U32-Hex (
                [Pcsx2TraceNative]::U32($ram, [int]$componentBase + 0x13c)))
            ui_label_width = [Math]::Round(
                [Pcsx2TraceNative]::F32($ram, [int]$componentBase + 0x140), 6)
            ui_label_height = [Math]::Round(
                [Pcsx2TraceNative]::F32($ram, [int]$componentBase + 0x144), 6)
            rnd_text_wrap_width = [Math]::Round(
                [Pcsx2TraceNative]::F32($ram, [int]$componentBase + 0x148), 6)
            component_pointer_fields = $componentPointers
            layout_words = $layoutWords
            vector_candidates = $vectorCandidates
          }
        }
      }
      $careerObjects += [ordered]@{
        name = $term
        live_component = $liveComponent
        hits = $hitItems
      }
    }
    $careerTracePath = Join-Path $OutDir "career_runtime_transform_trace.json"
    [ordered]@{
      generated_at = (Get-Date).ToString("s")
      source = "settled retail GH2 PS2 Career page in noninteractive PCSX2 memory trace"
      pcsx2_pid = $proc.Id
      ee_base_host = ("0x{0:x16}" -f $eeBase)
      readable_guest_ranges = $ramRead.readable_ranges
      no_focus_policy = "WindowStyle $OracleWindowStyle; input used PostMessage only"
      proof_capture = $(if ($NoProofCapture) { "disabled" } else { "enabled" })
      objects = $careerObjects
    } | ConvertTo-Json -Depth 20 |
        Set-Content -LiteralPath $careerTracePath -Encoding UTF8
    Write-Host $careerTracePath
    return
  }
  $terms = @(
    "sel_guitar_new_screen",
    "sel_guitar_panel",
    "guitar_display_panel",
    "show_guitar",
    "guitar.pxy",
    "guitar_single.filt",
    "guitar_single.tnm",
    "guitar.env",
    "lespaul",
    "lespaul_cherry",
    "sg_cherry",
    "guitar_single.view",
    "guitar_setup.cam"
  )
  $stringHits = [ordered]@{}
  foreach ($term in $terms) {
    $stringHits[$term] = @(String-Hits $ram $term | ForEach-Object { Guest-Hex $_ })
  }
  $scriptContexts = @()
  foreach ($term in @("guitar_display_panel", "show_guitar", "guitar.pxy", "guitar_single.filt", "guitar.env")) {
    foreach ($hit in @(String-Hits $ram $term | Select-Object -First 4)) {
      $scriptContexts += [ordered]@{
        term = $term
        guest = (Guest-Hex $hit)
        context = (Ascii-Window $ram $hit 96)
      }
    }
  }

  $pxyHits = @(String-Hits $ram "guitar.pxy")
  $filtHits = @(String-Hits $ram "guitar_single.filt")
  $tnmHits = @(String-Hits $ram "guitar_single.tnm")
  $argumentBlocks = @()
  if ($pxyHits.Count -gt 0 -and $filtHits.Count -gt 0) {
    $pxy = [uint32]$pxyHits[0]
    $filt = [uint32]$filtHits[0]
    $pxyPtrHits = @([Pcsx2TraceNative]::FindU32($ram, $pxy))
    foreach ($ptr in $pxyPtrHits | Select-Object -First 256) {
      for ($back = 0; $back -le 64; $back += 4) {
        $base = [int]$ptr - $back
        if ($base -lt 0) { continue }
        $foundFilter = $false
        for ($scan = 0; $scan -le 96; $scan += 4) {
          if ([Pcsx2TraceNative]::U32($ram, $base + $scan) -eq $filt) {
            $foundFilter = $true
            break
          }
        }
        if ($foundFilter) {
          $argumentBlocks += [ordered]@{
            base_guest = (Guest-Hex $base)
            pxy_pointer_at = (Guest-Hex $ptr)
            words = @(Pointer-Block $ram $base 36)
          }
          break
        }
      }
      if ($argumentBlocks.Count -ge 12) { break }
    }
  }

  $filterBodyPath = Join-Path $Repo "engine\out\menu_tuning\milo_inspect_20260703\sel_guitar_entries\AnimFilter__guitar_single.filt"
  $tnmBodyPath = Join-Path $Repo "engine\out\menu_tuning\milo_inspect_20260703\sel_guitar_entries\TransAnim__guitar_single.tnm"
  $proxyBodyPath = Join-Path $Repo "engine\out\menu_tuning\milo_inspect_20260703\sel_guitar_entries\UIProxy__guitar.pxy"
  $filterExactHits = @()
  $tnmExactHits = @()
  $proxyExactHits = @()
  if (Test-Path -LiteralPath $filterBodyPath) {
    $filterBody = [IO.File]::ReadAllBytes($filterBodyPath)
    $filterExactHits = @([Pcsx2TraceNative]::IndexOfAll($ram, $filterBody))
  }
  if (Test-Path -LiteralPath $tnmBodyPath) {
    $tnmBody = [IO.File]::ReadAllBytes($tnmBodyPath)
    $tnmExactHits = @([Pcsx2TraceNative]::IndexOfAll($ram, $tnmBody))
  }
  if (Test-Path -LiteralPath $proxyBodyPath) {
    $proxyBody = [IO.File]::ReadAllBytes($proxyBodyPath)
    $proxyExactHits = @([Pcsx2TraceNative]::IndexOfAll($ram, $proxyBody))
  }

  $liveFilter = $null
  if ($filterExactHits.Count -gt 0) {
    $fo = [int]$filterExactHits[0]
    $liveFilter = [ordered]@{
      body_guest = (Guest-Hex $fo)
      frame_header = [Math]::Round([Pcsx2TraceNative]::F32($ram, $fo + 0x10), 6)
      transanim_ref = [Pcsx2TraceNative]::AsciiAt($ram, $fo + 0x1c, 64)
      playback_rate = [Math]::Round([Pcsx2TraceNative]::F32($ram, $fo + 0x30), 6)
      range_end = [Math]::Round([Pcsx2TraceNative]::F32($ram, $fo + 0x3c), 6)
    }
  }

  $liveProxy = $null
  if ($proxyExactHits.Count -gt 0) {
    $po = [int]$proxyExactHits[0]
    $liveProxy = [ordered]@{
      body_guest = (Guest-Hex $po)
      local = (Decode-Matrix $ram ($po + 0x1b))
      world = (Decode-Matrix $ram ($po + 0x4b))
      parent = [Pcsx2TraceNative]::AsciiAt($ram, $po + 0x88, 64)
    }
  }

  $liveTnm = $null
  if ($tnmExactHits.Count -gt 0) {
    $to = [int]$tnmExactHits[0]
    $keys = @()
    foreach ($ko in @(0x3f, 0x53, 0x67)) {
      $keys += [ordered]@{
        guest = (Guest-Hex ($to + $ko))
        quat_xyzw = @(
          [Math]::Round([Pcsx2TraceNative]::F32($ram, $to + $ko + 0), 6),
          [Math]::Round([Pcsx2TraceNative]::F32($ram, $to + $ko + 4), 6),
          [Math]::Round([Pcsx2TraceNative]::F32($ram, $to + $ko + 8), 6),
          [Math]::Round([Pcsx2TraceNative]::F32($ram, $to + $ko + 12), 6)
        )
        frame = [Math]::Round([Pcsx2TraceNative]::F32($ram, $to + $ko + 16), 6)
      }
    }
    $liveTnm = [ordered]@{
      body_guest = (Guest-Hex $to)
      target = [Pcsx2TraceNative]::AsciiAt($ram, $to + 0x1d, 64)
      sampled_rotation_keys = $keys
    }
  }

  $liveNamedTransforms = @()
  foreach ($term in @(
      "guitar.pxy",
      "guitar.mesh",
      "guitar_single.view",
      "guitar_setup.cam",
      "meta.cam")) {
    $termHits = @()
    foreach ($hit in @(String-Hits $ram $term | Select-Object -First 8)) {
      $termRefs = @()
      foreach ($ref in @([Pcsx2TraceNative]::FindU32(
            $ram, [uint32]$hit) | Select-Object -First 16)) {
        $objectCandidate = [Pcsx2TraceNative]::U32($ram, [int]$ref + 4)
        $runtimeChild = if (Is-GuestPointer $objectCandidate $ram.Length) {
          [Pcsx2TraceNative]::U32($ram, [int]$objectCandidate + 0x144)
        } else { [uint32]0 }
        $attachmentCandidates = @()
        if (Is-GuestPointer $objectCandidate $ram.Length) {
          foreach ($attachmentOff in @(0x170, 0x180, 0x194)) {
            $attachmentPtr = [Pcsx2TraceNative]::U32(
                $ram, [int]$objectCandidate + $attachmentOff)
            $attachmentCandidates += [ordered]@{
              field_offset = ("0x{0:x}" -f $attachmentOff)
              pointer = (U32-Hex $attachmentPtr)
              transform_candidates = $(if (
                  Is-GuestPointer $attachmentPtr $ram.Length) {
                    @(Runtime-Transform-CandidatesNear $ram $attachmentPtr 0x2000 64)
                  } else { @() })
            }
          }
        }
        $termRefs += [ordered]@{
          ref_guest = (Guest-Hex $ref)
          nearby_words = @(Float-Block $ram ([int]$ref - 0x20) 24)
          directory_object_candidate = (U32-Hex $objectCandidate)
          object_transform_candidates = $(if (
              Is-GuestPointer $objectCandidate $ram.Length) {
                @(Runtime-Transform-CandidatesNear $ram $objectCandidate 0x400 24)
              } else { @() })
          object_pointer_fields = $(if (
              Is-GuestPointer $objectCandidate $ram.Length) {
                @(Pointer-Fields $ram ([int]$objectCandidate) 0x240 64)
              } else { @() })
          runtime_child_0x144 = (U32-Hex $runtimeChild)
          runtime_child_transform_candidates = $(if (
              Is-GuestPointer $runtimeChild $ram.Length) {
                @(Runtime-Transform-CandidatesNear $ram $runtimeChild 0x600 32)
              } else { @() })
          attachment_candidates = $attachmentCandidates
          transform_candidates = @(
              Runtime-Transform-CandidatesNear $ram ([uint32]$ref) 0x300 16)
        }
      }
      $termHits += [ordered]@{
        string_guest = (Guest-Hex $hit)
        near_string_transform_candidates = @(
            Runtime-Transform-CandidatesNear $ram (
                [uint32][Math]::Max(0, $hit - 0x800)) 0x1000 48)
        references = $termRefs
      }
    }
    $liveNamedTransforms += [ordered]@{
      name = $term
      hits = $termHits
    }
  }

  $trace = [ordered]@{
    generated_at = (Get-Date).ToString("s")
    pcsx2_pid = $proc.Id
    ee_base_host = ("0x{0:x16}" -f $eeBase)
    readable_guest_ranges = $ramRead.readable_ranges
    no_focus_policy = "launched with WindowStyle Hidden; input used PostMessage to PCSX2-owned windows only; no foreground activation"
    setup_cross_count = $SetupCrossCount
    post_career_cross_count = $PostCareerCrossCount
    fresh_boot = [bool]$FreshBoot
    skip_hook = [bool]$SkipHook
    route = @(
      "load state slot 1",
      "cross retry",
      "start pause",
      "down x4 to quit",
      "cross quit and cross confirm",
      "triangle back to main",
      "up to career, cross",
      "name-band down once, cross x8, start",
      "cross through save/difficulty/character to guitar select"
    )
    proof_image = $(if ($freshSnapProof -and (Test-Path -LiteralPath $snapProofPng)) { $snapProofPng } else { $proofPng })
    proof_image_is_native_pcsx2_snap = $freshSnapProof
    step_proofs = $stepProofs
    printwindow_image = $proofPng
    string_hits = $stringHits
    script_contexts = $scriptContexts
    show_guitar_argument_blocks = $argumentBlocks
    show_guitar_runtime_code_trace = (Trace-ShowGuitarCode $ram)
    display_setup_hook_install = $setupHook
    display_setup_hook_trace = (Read-DisplaySetupHookTrace $ram)
    guitar_display_entry_trace = (Trace-GuitarDisplayEntries $ram)
    live_named_transform_candidates = $liveNamedTransforms
    live_loaded_assets = [ordered]@{
      filter_exact_hits = @($filterExactHits | Select-Object -First 8 | ForEach-Object { Guest-Hex $_ })
      transanim_exact_hits = @($tnmExactHits | Select-Object -First 8 | ForEach-Object { Guest-Hex $_ })
      proxy_exact_hits = @($proxyExactHits | Select-Object -First 8 | ForEach-Object { Guest-Hex $_ })
      guitar_single_filter = $liveFilter
      guitar_single_transanim = $liveTnm
      guitar_proxy = $liveProxy
    }
  }
  $trace | ConvertTo-Json -Depth 20 | Set-Content -LiteralPath $traceJson -Encoding UTF8
  Write-Host $traceJson
} finally {
  if ($proc -and -not $proc.HasExited) {
    $proc.CloseMainWindow() | Out-Null
    Start-Sleep -Seconds 2
    if (-not $proc.HasExited) { Stop-Process -Id $proc.Id -Force }
  }
  if ($null -ne $pcsxIniOriginal) {
    [IO.File]::WriteAllBytes($pcsxIniPath, $pcsxIniOriginal)
  }
}
