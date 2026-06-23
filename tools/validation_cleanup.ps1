param(
    [string]$Root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path,
    [switch]$DeleteTraceStaging,
    [int]$Top = 15
)

$ErrorActionPreference = "Stop"

$resolvedRoot = (Resolve-Path -LiteralPath $Root).Path

function Format-MB([long]$bytes) {
    return [math]::Round($bytes / 1MB, 2)
}

Write-Host "Validation cleanup audit root: $resolvedRoot"

$traceDirs = @(Get-ChildItem -LiteralPath $resolvedRoot -Recurse -Force -Directory -Filter "GH2DXu_PS2_trace_*" -ErrorAction SilentlyContinue)
$traceImages = @(Get-ChildItem -LiteralPath $resolvedRoot -Recurse -Force -File -ErrorAction SilentlyContinue |
    Where-Object { $_.Name -match "GH2DXu_PS2_trace_.*\.(iso|mds)$" })

if ($traceDirs.Count -eq 0 -and $traceImages.Count -eq 0) {
    Write-Host "Trace staging artifacts: none"
} else {
    Write-Host "Trace staging directories:"
    $traceDirs | Select-Object FullName, LastWriteTime | Format-Table -AutoSize -Wrap
    Write-Host "Trace ISO/MDS artifacts:"
    $traceImages | Select-Object FullName, Length, LastWriteTime | Format-Table -AutoSize -Wrap
}

if ($DeleteTraceStaging) {
    foreach ($file in $traceImages) {
        Remove-Item -LiteralPath $file.FullName -Force
    }
    foreach ($dir in $traceDirs) {
        Remove-Item -LiteralPath $dir.FullName -Recurse -Force
    }
    Write-Host "Deleted trace staging artifacts: dirs=$($traceDirs.Count) files=$($traceImages.Count)"
}

$analysisRoot = Join-Path $resolvedRoot "analysis"
if (Test-Path -LiteralPath $analysisRoot) {
    Write-Host "Largest analysis buckets:"
    Get-ChildItem -LiteralPath $analysisRoot -Force -Directory -ErrorAction SilentlyContinue |
        ForEach-Object {
            $size = (Get-ChildItem -LiteralPath $_.FullName -Recurse -Force -File -ErrorAction SilentlyContinue |
                Measure-Object -Property Length -Sum).Sum
            [PSCustomObject]@{ Name = $_.Name; MB = Format-MB([long]$size) }
        } |
        Sort-Object MB -Descending |
        Select-Object -First $Top |
        Format-Table -AutoSize
}

Write-Host "Largest files:"
Get-ChildItem -LiteralPath $resolvedRoot -Recurse -Force -File -ErrorAction SilentlyContinue |
    Sort-Object Length -Descending |
    Select-Object -First $Top @{Name="MB"; Expression={Format-MB([long]$_.Length)}}, FullName, LastWriteTime |
    Format-Table -AutoSize -Wrap

$drive = Get-PSDrive -Name ((Get-Item -LiteralPath $resolvedRoot).PSDrive.Name)
Write-Host ("Drive {0}: free={1} MB used={2} MB" -f $drive.Name, (Format-MB([long]$drive.Free)), (Format-MB([long]$drive.Used)))
