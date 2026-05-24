param(
  [int]$MaxIter = 30,
  [int]$RunSeconds = 15
)
$ErrorActionPreference = 'Continue'

$root = "C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX"
$rexglue = "C:\Programming\GitHub\Guitar Hero II\rexglue-sdk\out\install\win-amd64\bin\rexglue.exe"
$env_bat = "$root\build_env.bat"
$manifest = "$root\gh2test_manifest.toml"
$config = "$root\gh2test_config.toml"
$exe = "$root\out\build\win-amd64-release\gh2test.exe"
$logs_dir = "$root\out\build\win-amd64-release\logs"
$game_arg = '--game_data_root="C:\Programming\GitHub\Guitar Hero II\GuitarHeroOGX\assets"'

# Parse legacy config into a hashtable: addr_int -> @{start=int, size=int}
function Load-LegacyTable {
  $tbl = @{}
  Get-Content $config | ForEach-Object {
    if ($_ -match '^0x([0-9a-fA-F]+) = \{ name = "[^"]+", size = 0x([0-9a-fA-F]+)') {
      $addr = [Convert]::ToUInt32($Matches[1], 16)
      $sz   = [Convert]::ToUInt32($Matches[2], 16)
      $tbl[$addr] = @{start=$addr; size=$sz}
    }
  }
  $tbl
}

function Find-Parent($table, $addrHex) {
  $addr = [Convert]::ToUInt32($addrHex, 16)
  foreach ($k in $table.Keys) {
    $e = $table[$k]
    if ($addr -ge $e.start -and $addr -lt ($e.start + $e.size)) {
      return @{ parent = "0x{0:X8}" -f $e.start; size = $e.size }
    }
  }
  return $null
}

function Get-CurrentChunks {
  $chunks = @{}
  Get-Content $manifest | ForEach-Object {
    if ($_ -match '^(0x[0-9a-fA-F]+) = \{ name = "[^"]+", parent = (0x[0-9a-fA-F]+)') {
      $chunks[$Matches[1].ToUpper()] = $Matches[2]
    }
  }
  $chunks
}

function Append-Chunk($addrHex, $parentHex) {
  $line = '{0} = {{ name = "rex_sub_{1}", parent = {2} }}' -f $addrHex, $addrHex.Substring(2), $parentHex
  Add-Content -Path $manifest -Value $line
}

function Run-Codegen {
  $clog = "$root\codegen.log"
  & $env_bat $rexglue -f codegen $manifest *> $clog
  if ($LASTEXITCODE -ne 0) { throw "Codegen failed exit 0x{0:X8}; tail:`n$(Get-Content $clog -Tail 8 | Out-String)" -f $LASTEXITCODE }
}

function Run-Build {
  $blog = "$root\build.log"
  & $env_bat cmake --build "$root\out\build\win-amd64-release" --target gh2test *> $blog
  if ($LASTEXITCODE -ne 0) { throw "Build failed; tail:`n$(Get-Content $blog -Tail 8 | Out-String)" }
}

function Run-Game {
  $p = Start-Process -FilePath $exe -ArgumentList $game_arg -WorkingDirectory (Split-Path $exe) -PassThru
  $sw = [Diagnostics.Stopwatch]::StartNew()
  while ($sw.Elapsed.TotalSeconds -lt $RunSeconds) {
    Start-Sleep -Milliseconds 250
    $still = Get-Process -Id $p.Id -ErrorAction SilentlyContinue
    if (-not $still) { break }
  }
  $still = Get-Process -Id $p.Id -ErrorAction SilentlyContinue
  $alive = $false
  if ($still) {
    $alive = $true
    Stop-Process -Id $p.Id -Force
  }
  $latest = Get-ChildItem "$logs_dir\gh2test_*.log" | Sort-Object LastWriteTime -Descending | Select-Object -First 1
  return @{ alive = $alive; log = $latest.FullName }
}

function Find-MissingFunction($logFile) {
  $lines = Get-Content $logFile -Tail 20
  foreach ($l in $lines) {
    if ($l -match 'Call to invalid or unregistered function at guest address (0x[0-9a-fA-F]+)') {
      return $Matches[1].ToUpper()
    }
  }
  return $null
}

Write-Host "Loading legacy table..."
$table = Load-LegacyTable
Write-Host "Loaded $($table.Count) legacy entries"

for ($i = 1; $i -le $MaxIter; $i++) {
  Write-Host "`n=== Iter $i ==="
  Write-Host "Codegen..."
  Run-Codegen
  Write-Host "Build..."
  Run-Build
  Write-Host "Run..."
  $result = Run-Game
  if ($result.alive) {
    Write-Host "SUCCESS: app still alive after $RunSeconds s (no FATAL in window). Log: $($result.log)"
    return
  }
  $missing = Find-MissingFunction $result.log
  if (-not $missing) {
    Write-Host "App died but no FATAL function in log tail. Manual inspection needed."
    Write-Host "Log: $($result.log)"
    Write-Host "Last 15 lines:"
    Get-Content $result.log -Tail 15
    return
  }
  Write-Host "Missing function: $missing"
  $chunks = Get-CurrentChunks
  if ($chunks.ContainsKey($missing)) {
    Write-Host "Already in manifest as chunk of $($chunks[$missing]) -- different crash. Log: $($result.log)"
    Get-Content $result.log -Tail 15
    return
  }
  $parentInfo = Find-Parent $table $missing
  if (-not $parentInfo) {
    Write-Host "Address $missing not contained in any legacy function. Adding as standalone."
    $line = '{0} = {{ name = "rex_sub_{1}" }}' -f $missing, $missing.Substring(2)
    Add-Content -Path $manifest -Value $line
  } else {
    Write-Host "Adding chunk $missing -> parent $($parentInfo.parent)"
    Append-Chunk $missing $parentInfo.parent
  }
}
Write-Host "Hit max iterations $MaxIter without success."
