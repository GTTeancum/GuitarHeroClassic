[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$SingerMilo,
    [Parameter(Mandatory = $true)][string]$DonorMilo,
    [Parameter(Mandatory = $true)][string]$OutputBlend,
    [string[]]$SingerExtraMilo = @(),
    [string[]]$DonorExtraMilo = @(),
    [string]$Milo2GltfExe,
    [string]$RigExportExe,
    [string]$BlenderExe = 'C:\Program Files\Blender Foundation\Blender 4.5\blender.exe',
    [string]$SingerArmature,
    [string]$DonorArmature,
    [string[]]$ExcludeSubtreeRoot = @(),
    [ValidateSet('bounds', 'uniform', 'none')][string]$ScaleMode = 'bounds'
)

$ErrorActionPreference = 'Stop'
$toolRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$stageScript = Join-Path $toolRoot 'stage_finger_rig.py'
$workspace = (Resolve-Path (Join-Path $toolRoot '..\..')).Path

if (-not $Milo2GltfExe) {
    $Milo2GltfExe = Join-Path $workspace '..\ihatecompvir-public-milo-sources\grim\target\release\mesh_tool.exe'
}
if (-not $RigExportExe) {
    $RigExportExe = Join-Path $toolRoot 'build\Release\milo_rig_export.exe'
}
foreach ($required in @($SingerMilo, $DonorMilo, $Milo2GltfExe, $RigExportExe, $BlenderExe, $stageScript) + $SingerExtraMilo + $DonorExtraMilo) {
    if (-not (Test-Path -LiteralPath $required -PathType Leaf)) {
        throw "Required file not found: $required"
    }
}

$scratch = Join-Path ([IO.Path]::GetTempPath()) ('ghogx-finger-stage-' + [guid]::NewGuid().ToString('N'))
$singerDir = Join-Path $scratch 'singer'
$donorDir = Join-Path $scratch 'donor'
New-Item -ItemType Directory -Path $singerDir, $donorDir | Out-Null

try {
    $singerConvertArgs = @('milo2gltf', $SingerMilo) + $SingerExtraMilo + @('-o', $singerDir, '-n', 'singer', '-b')
    $donorConvertArgs = @('milo2gltf', $DonorMilo) + $DonorExtraMilo + @('-o', $donorDir, '-n', 'donor', '-b')
    & $Milo2GltfExe @singerConvertArgs
    if ($LASTEXITCODE -ne 0) { throw "Singer Milo-to-glTF conversion failed: $LASTEXITCODE" }
    & $Milo2GltfExe @donorConvertArgs
    if ($LASTEXITCODE -ne 0) { throw "Donor Milo-to-glTF conversion failed: $LASTEXITCODE" }

    $singerRigPath = Join-Path $singerDir 'singer.rig.json'
    $donorRigPath = Join-Path $donorDir 'donor.rig.json'
    & $RigExportExe $SingerMilo $singerRigPath
    if ($LASTEXITCODE -ne 0) { throw "Singer native-rig export failed: $LASTEXITCODE" }
    & $RigExportExe $DonorMilo $donorRigPath
    if ($LASTEXITCODE -ne 0) { throw "Donor native-rig export failed: $LASTEXITCODE" }

    $singerScenes = @(Get-ChildItem -LiteralPath $singerDir -Recurse -File |
        Where-Object { $_.Extension -in @('.gltf', '.glb') })
    $donorScenes = @(Get-ChildItem -LiteralPath $donorDir -Recurse -File |
        Where-Object { $_.Extension -in @('.gltf', '.glb') })
    if ($singerScenes.Count -ne 1) { throw "Expected one singer glTF scene, found $($singerScenes.Count)" }
    if ($donorScenes.Count -ne 1) { throw "Expected one donor glTF scene, found $($donorScenes.Count)" }
    $singerScenePath = $singerScenes[0].FullName
    $donorScenePath = $donorScenes[0].FullName
    $outputFullPath = [IO.Path]::GetFullPath($OutputBlend)

    $blenderArgs = @(
        '--background', '--python-exit-code', '1', '--python', $stageScript, '--',
        '--singer', $singerScenePath,
        '--donor', $donorScenePath,
        '--singer-rig', $singerRigPath,
        '--donor-rig', $donorRigPath,
        '--output', $outputFullPath,
        '--scale-mode', $ScaleMode
    )
    if ($SingerArmature) { $blenderArgs += @('--singer-armature', $SingerArmature) }
    if ($DonorArmature) { $blenderArgs += @('--donor-armature', $DonorArmature) }
    foreach ($subtree in $ExcludeSubtreeRoot) {
        $blenderArgs += @('--exclude-subtree-root', $subtree)
    }
    & $BlenderExe @blenderArgs
    if ($LASTEXITCODE -ne 0) { throw "Blender finger staging failed: $LASTEXITCODE" }
    if (-not (Test-Path -LiteralPath $outputFullPath -PathType Leaf)) {
        throw "Blender reported success but did not create: $outputFullPath"
    }
}
finally {
    if (Test-Path -LiteralPath $scratch) {
        Remove-Item -LiteralPath $scratch -Recurse -Force
    }
}
