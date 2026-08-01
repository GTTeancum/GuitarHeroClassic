param(
  [Parameter(Mandatory = $true, Position = 0, ValueFromRemainingArguments = $true)]
  [string[]]$Path
)

$ErrorActionPreference = "Stop"

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

function Read-UInt16LE {
  param(
    [Parameter(Mandatory = $true)][byte[]]$Bytes,
    [Parameter(Mandatory = $true)][int]$Offset
  )
  if ($Offset -lt 0 -or $Offset + 2 -gt $Bytes.Length) {
    throw "PE read past end at offset $Offset"
  }
  return [System.BitConverter]::ToUInt16($Bytes, $Offset)
}

function Read-UInt32LE {
  param(
    [Parameter(Mandatory = $true)][byte[]]$Bytes,
    [Parameter(Mandatory = $true)][int]$Offset
  )
  if ($Offset -lt 0 -or $Offset + 4 -gt $Bytes.Length) {
    throw "PE read past end at offset $Offset"
  }
  return [System.BitConverter]::ToUInt32($Bytes, $Offset)
}

function Read-NullTerminatedAscii {
  param(
    [Parameter(Mandatory = $true)][byte[]]$Bytes,
    [Parameter(Mandatory = $true)][int]$Offset
  )
  if ($Offset -lt 0 -or $Offset -ge $Bytes.Length) {
    throw "PE string offset out of range: $Offset"
  }

  $end = $Offset
  while ($end -lt $Bytes.Length -and $Bytes[$end] -ne 0) {
    $end++
  }
  if ($end -ge $Bytes.Length) {
    throw "PE string at offset $Offset is not null terminated"
  }

  return [System.Text.Encoding]::ASCII.GetString($Bytes, $Offset, $end - $Offset)
}

function Convert-RvaToFileOffset {
  param(
    [Parameter(Mandatory = $true)][uint32]$Rva,
    [Parameter(Mandatory = $true)]$Sections,
    [Parameter(Mandatory = $true)][int]$FileLength
  )

  foreach ($section in $Sections) {
    $span = [Math]::Max([uint32]$section.VirtualSize, [uint32]$section.SizeOfRawData)
    if ($span -eq 0) {
      continue
    }

    if ($Rva -ge $section.VirtualAddress -and
        $Rva -lt ($section.VirtualAddress + $span)) {
      $offset = [int]($section.PointerToRawData + ($Rva - $section.VirtualAddress))
      if ($offset -lt 0 -or $offset -ge $FileLength) {
        throw "PE RVA 0x$($Rva.ToString('x')) maps outside the file"
      }
      return $offset
    }
  }

  if ($Rva -lt [uint32]$FileLength) {
    return [int]$Rva
  }

  throw "Could not map PE RVA 0x$($Rva.ToString('x')) to a file offset"
}

function Add-PeImportDirectoryDlls {
  param(
    [Parameter(Mandatory = $true)][byte[]]$Bytes,
    [Parameter(Mandatory = $true)]$Sections,
    [Parameter(Mandatory = $true)][uint32]$DirectoryRva,
    [Parameter(Mandatory = $true)][uint32]$DirectorySize,
    [System.Collections.Generic.HashSet[string]]$Dlls,
    [Parameter(Mandatory = $true)][int]$DescriptorSize,
    [Parameter(Mandatory = $true)][int]$NameRvaOffset
  )

  if ($DirectoryRva -eq 0) {
    return
  }

  $descriptor = Convert-RvaToFileOffset -Rva $DirectoryRva -Sections $Sections -FileLength $Bytes.Length
  $maxDescriptors = 4096
  if ($DirectorySize -ge [uint32]$DescriptorSize) {
    $maxDescriptors = [Math]::Min($maxDescriptors, [int]($DirectorySize / [uint32]$DescriptorSize) + 1)
  }

  for ($i = 0; $i -lt $maxDescriptors; $i++) {
    $offset = $descriptor + ($i * $DescriptorSize)
    if ($offset + $DescriptorSize -gt $Bytes.Length) {
      throw "PE import descriptor table runs past end of file"
    }

    $allZero = $true
    for ($j = 0; $j -lt $DescriptorSize; $j += 4) {
      if ((Read-UInt32LE -Bytes $Bytes -Offset ($offset + $j)) -ne 0) {
        $allZero = $false
        break
      }
    }
    if ($allZero) {
      return
    }

    $nameRva = Read-UInt32LE -Bytes $Bytes -Offset ($offset + $NameRvaOffset)
    if ($nameRva -ne 0) {
      $nameOffset = Convert-RvaToFileOffset -Rva $nameRva -Sections $Sections -FileLength $Bytes.Length
      [void]$Dlls.Add((Read-NullTerminatedAscii -Bytes $Bytes -Offset $nameOffset))
    }
  }

  throw "PE import descriptor table did not terminate"
}

function Get-PeImportedDlls {
  param(
    [Parameter(Mandatory = $true)][string]$Exe
  )

  $bytes = [System.IO.File]::ReadAllBytes($Exe)
  if ($bytes.Length -lt 0x40) {
    throw "$Exe is too small to be a PE executable"
  }
  if ($bytes[0] -ne 0x4d -or $bytes[1] -ne 0x5a) {
    throw "$Exe is not a PE executable"
  }

  $peOffset = [int](Read-UInt32LE -Bytes $bytes -Offset 0x3c)
  if ($peOffset + 24 -gt $bytes.Length) {
    throw "$Exe has an invalid PE header offset"
  }
  if ($bytes[$peOffset] -ne 0x50 -or $bytes[$peOffset + 1] -ne 0x45 -or
      $bytes[$peOffset + 2] -ne 0 -or $bytes[$peOffset + 3] -ne 0) {
    throw "$Exe has an invalid PE signature"
  }

  $coffOffset = $peOffset + 4
  $sectionCount = Read-UInt16LE -Bytes $bytes -Offset ($coffOffset + 2)
  $optionalHeaderSize = Read-UInt16LE -Bytes $bytes -Offset ($coffOffset + 16)
  $optionalOffset = $coffOffset + 20
  $sectionOffset = $optionalOffset + $optionalHeaderSize
  if ($sectionOffset + ($sectionCount * 40) -gt $bytes.Length) {
    throw "$Exe has section headers outside the file"
  }

  $magic = Read-UInt16LE -Bytes $bytes -Offset $optionalOffset
  if ($magic -eq 0x10b) {
    $dataDirectoryOffset = $optionalOffset + 96
  } elseif ($magic -eq 0x20b) {
    $dataDirectoryOffset = $optionalOffset + 112
  } else {
    throw "$Exe has unsupported PE optional header magic 0x$($magic.ToString('x'))"
  }
  if ($dataDirectoryOffset + (14 * 8) -gt $optionalOffset + $optionalHeaderSize) {
    throw "$Exe does not contain enough PE data directories"
  }

  $sections = @()
  for ($i = 0; $i -lt $sectionCount; $i++) {
    $offset = $sectionOffset + ($i * 40)
    $sections += [pscustomobject]@{
      VirtualSize      = Read-UInt32LE -Bytes $bytes -Offset ($offset + 8)
      VirtualAddress   = Read-UInt32LE -Bytes $bytes -Offset ($offset + 12)
      SizeOfRawData    = Read-UInt32LE -Bytes $bytes -Offset ($offset + 16)
      PointerToRawData = Read-UInt32LE -Bytes $bytes -Offset ($offset + 20)
    }
  }

  $dlls = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::OrdinalIgnoreCase)

  $importDirectoryOffset = $dataDirectoryOffset + 8
  Add-PeImportDirectoryDlls -Bytes $bytes -Sections $sections `
    -DirectoryRva (Read-UInt32LE -Bytes $bytes -Offset $importDirectoryOffset) `
    -DirectorySize (Read-UInt32LE -Bytes $bytes -Offset ($importDirectoryOffset + 4)) `
    -Dlls $dlls -DescriptorSize 20 -NameRvaOffset 12

  $delayImportDirectoryOffset = $dataDirectoryOffset + (13 * 8)
  Add-PeImportDirectoryDlls -Bytes $bytes -Sections $sections `
    -DirectoryRva (Read-UInt32LE -Bytes $bytes -Offset $delayImportDirectoryOffset) `
    -DirectorySize (Read-UInt32LE -Bytes $bytes -Offset ($delayImportDirectoryOffset + 4)) `
    -Dlls $dlls -DescriptorSize 32 -NameRvaOffset 4

  return @($dlls | Sort-Object)
}

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
  $dlls = @(Get-PeImportedDlls -Exe $exe)

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
