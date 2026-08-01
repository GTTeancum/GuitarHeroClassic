[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string] $InputPath,

    [Parameter(Mandatory = $true)]
    [string] $OutputPath,

    [double] $X = 0.0,
    [double] $Y = 0.0,
    [double] $Z = 0.0,

    [Nullable[double]] $StoredX,
    [Nullable[double]] $StoredY,
    [Nullable[double]] $StoredZ
)

$ErrorActionPreference = 'Stop'
$inputFullPath = (Resolve-Path -LiteralPath $InputPath).Path
$outputFullPath = [IO.Path]::GetFullPath($OutputPath)
$bytes = [IO.File]::ReadAllBytes($inputFullPath)

# A GH2 PS2 standalone revision-28 Mesh begins with the revision followed by
# the nine-byte Object/meta prefix. Its local 3x4 transform starts at 0x11,
# placing M41/M42/M43 at 0x35/0x39/0x3d. Preserve every other byte.
if ($bytes.Length -lt 0x41) {
    throw "Mesh is too short to contain a standalone transform: $inputFullPath"
}
$revision = [BitConverter]::ToInt32($bytes, 0)
if ($revision -ne 28) {
    throw "Expected a GH2 PS2 revision-28 Mesh, found revision $revision"
}

$translationOffset = 0x35
$oldX = [BitConverter]::ToSingle($bytes, $translationOffset)
$oldY = [BitConverter]::ToSingle($bytes, $translationOffset + 4)
$oldZ = [BitConverter]::ToSingle($bytes, $translationOffset + 8)
$newX = [single]($oldX + $X)
$newY = [single]($oldY + $Y)
$newZ = [single]($oldZ + $Z)

[BitConverter]::GetBytes($newX).CopyTo($bytes, $translationOffset)
[BitConverter]::GetBytes($newY).CopyTo($bytes, $translationOffset + 4)
[BitConverter]::GetBytes($newZ).CopyTo($bytes, $translationOffset + 8)

$storedKeys = @('StoredX', 'StoredY', 'StoredZ') |
    Where-Object { $PSBoundParameters.ContainsKey($_) }
if ($storedKeys.Count -ne 0 -and $storedKeys.Count -ne 3) {
    throw 'StoredX, StoredY, and StoredZ must be supplied together.'
}

$storedMessage = ''
if ($storedKeys.Count -eq 3) {
    # The resolved/stored world transform immediately follows the local matrix.
    # Its translation starts at 0x65. The caller supplies the already-rotated
    # world-space delta because the parent transform is outside this object.
    $storedOffset = 0x65
    $oldStoredX = [BitConverter]::ToSingle($bytes, $storedOffset)
    $oldStoredY = [BitConverter]::ToSingle($bytes, $storedOffset + 4)
    $oldStoredZ = [BitConverter]::ToSingle($bytes, $storedOffset + 8)
    $newStoredX = [single]($oldStoredX + $StoredX)
    $newStoredY = [single]($oldStoredY + $StoredY)
    $newStoredZ = [single]($oldStoredZ + $StoredZ)
    [BitConverter]::GetBytes($newStoredX).CopyTo($bytes, $storedOffset)
    [BitConverter]::GetBytes($newStoredY).CopyTo($bytes, $storedOffset + 4)
    [BitConverter]::GetBytes($newStoredZ).CopyTo($bytes, $storedOffset + 8)
    $storedFormat =
        " stored_old=({0:R},{1:R},{2:R})" +
        " stored_delta=({3:R},{4:R},{5:R})" +
        " stored_new=({6:R},{7:R},{8:R})"
    $storedMessage = $storedFormat -f
        $oldStoredX, $oldStoredY, $oldStoredZ,
        $StoredX, $StoredY, $StoredZ,
        $newStoredX, $newStoredY, $newStoredZ
}

$parent = Split-Path -Parent $outputFullPath
if ($parent) {
    [IO.Directory]::CreateDirectory($parent) | Out-Null
}
[IO.File]::WriteAllBytes($outputFullPath, $bytes)

$message =
    "OFFSET_PS2_MESH old=({0:R},{1:R},{2:R}) delta=({3:R},{4:R},{5:R}) " +
    "new=({6:R},{7:R},{8:R}){9} output={10}"
Write-Output (
    $message -f
    $oldX, $oldY, $oldZ,
    $X, $Y, $Z,
    $newX, $newY, $newZ,
    $storedMessage,
    $outputFullPath
)
