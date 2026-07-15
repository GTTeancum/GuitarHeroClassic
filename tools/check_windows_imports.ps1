param(
  [Parameter(Mandatory = $true, Position = 0, ValueFromRemainingArguments = $true)]
  [string[]]$Path
)

$ErrorActionPreference = "Stop"

$dumpbin = Get-Command dumpbin.exe -ErrorAction SilentlyContinue
if (-not $dumpbin) {
  throw "dumpbin.exe was not found. Run this from build_env.bat so the MSVC proof tools are on PATH."
}

$forbidden = @(
  '^libc\+\+.*\.dll$',
  '^libunwind.*\.dll$',
  '^libgcc.*\.dll$',
  '^libstdc\+\+.*\.dll$',
  '^msvcp.*\.dll$',
  '^vcruntime.*\.dll$',
  '^ucrtbase\.dll$',
  '^concrt.*\.dll$',
  '^api-ms-win-crt-.*\.dll$'
)

$scan = New-Object System.Collections.Generic.List[string]
foreach ($item in $Path) {
  $resolved = Resolve-Path -LiteralPath $item
  foreach ($match in $resolved) {
    if ((Get-Item -LiteralPath $match.Path).PSIsContainer) {
      Get-ChildItem -LiteralPath $match.Path -Recurse -Filter *.exe |
        Where-Object { $_.FullName -notmatch '\\CMakeFiles\\' } |
        ForEach-Object { $scan.Add($_.FullName) }
    } else {
      $scan.Add($match.Path)
    }
  }
}

$scan = $scan | Sort-Object -Unique
if ($scan.Count -eq 0) {
  throw "No executable files were found to check."
}

$failed = $false

foreach ($exe in $scan) {
  $output = & $dumpbin.Source /dependents $exe 2>&1
  if ($LASTEXITCODE -ne 0) {
    $output | ForEach-Object { Write-Error $_ }
    throw "dumpbin failed for $exe"
  }

  $dlls = @()
  foreach ($line in $output) {
    if ($line -match '^\s+([A-Za-z0-9_.+\-]+\.dll)\s*$') {
      $dlls += $matches[1]
    }
  }
  $dlls = $dlls | Sort-Object -Unique

  $bad = @()
  foreach ($dll in $dlls) {
    foreach ($pattern in $forbidden) {
      if ($dll -match $pattern) {
        $bad += $dll
        break
      }
    }
  }
  $bad = $bad | Sort-Object -Unique

  if ($bad.Count -gt 0) {
    Write-Error "$exe imports forbidden non-platform runtime DLL(s): $($bad -join ', ')"
    $failed = $true
  } else {
    Write-Host "$exe imports: $($dlls -join ', ')"
  }
}

if ($failed) {
  exit 1
}
