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

$failed = $false

foreach ($item in $Path) {
  $exe = (Resolve-Path -LiteralPath $item).Path
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
