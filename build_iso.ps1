# build_iso.ps1 - Generates a compliant El-Torito Bootable ISO 9660 image
# Standalone, requires no external Linux tools or packages.
$ErrorActionPreference = "Stop"

$isoPath = "lohithos.iso"
$stage1Path = "boot/stage1_iso.bin"
$kernelPath = "kernel.bin"

if (-not (Test-Path $stage1Path)) {
    throw "Stage 1 bootloader missing: $stage1Path"
}
if (-not (Test-Path $kernelPath)) {
    throw "Kernel binary missing: $kernelPath"
}

$stage1Bytes = [System.IO.File]::ReadAllBytes($stage1Path)
$kernelBytes = [System.IO.File]::ReadAllBytes($kernelPath)

$sectorSize = 2048

# Calculate layout in 2048-byte sectors
# Sectors 0..15: System area (16 sectors = 32KB)
# Sector 16: Primary Volume Descriptor (PVD)
# Sector 17: Boot Record Volume Descriptor (BRVD)
# Sector 18: Volume Descriptor Set Terminator (VDST)
# Sector 19: Boot Catalog
# Sector 20: Stage 1 Boot Sector (1 sector = 2048 bytes)
# Sector 21..N: Kernel binary
$bootSectorLBA = 20
$kernelLBA = 21

$kernelSectorCount = [int][Math]::Ceiling($kernelBytes.Length / $sectorSize)
$totalSectors = 22 + $kernelSectorCount + 10 # Padded total sectors
$totalBytes = $totalSectors * $sectorSize

$isoBuffer = New-Object byte[] $totalBytes

function Set-Ascii($buffer, $offset, $text, $len) {
    $bytes = [System.Text.Encoding]::ASCII.GetBytes($text)
    for ($i = 0; $i -lt $len; $i++) {
        if ($i -lt $bytes.Length) {
            $buffer[$offset + $i] = $bytes[$i]
        } else {
            $buffer[$offset + $i] = 0x20 # Space pad
        }
    }
}

# =========================================================================
# Sector 16: Primary Volume Descriptor (PVD)
# =========================================================================
$pvdOff = 16 * $sectorSize
$isoBuffer[$pvdOff + 0] = 0x01 # PVD Type
Set-Ascii $isoBuffer ($pvdOff + 1) "CD001" 5
$isoBuffer[$pvdOff + 6] = 0x01 # Version
Set-Ascii $isoBuffer ($pvdOff + 8) "LOHITH_SYSTEMS" 32
Set-Ascii $isoBuffer ($pvdOff + 40) "LOHITHOS" 32

# Volume Space Size (Sectors) - Both Little and Big Endian
[System.BitConverter]::GetBytes([uint32]$totalSectors).CopyTo($isoBuffer, $pvdOff + 80)
$beSectors = [System.BitConverter]::GetBytes([uint32]$totalSectors)
[Array]::Reverse($beSectors)
$beSectors.CopyTo($isoBuffer, $pvdOff + 84)

# Logical Block Size (2048) - Little and Big Endian
$isoBuffer[$pvdOff + 128] = 0x00
$isoBuffer[$pvdOff + 129] = 0x08
$isoBuffer[$pvdOff + 130] = 0x08
$isoBuffer[$pvdOff + 131] = 0x00

# =========================================================================
# Sector 17: Boot Record Volume Descriptor (El Torito)
# =========================================================================
$brvdOff = 17 * $sectorSize
$isoBuffer[$brvdOff + 0] = 0x00 # Boot Record Type
Set-Ascii $isoBuffer ($brvdOff + 1) "CD001" 5
$isoBuffer[$brvdOff + 6] = 0x01 # Version

# Boot System Identifier (32 bytes)
$elToritoID = "EL TORITO SPECIFICATION"
$idBytes = [System.Text.Encoding]::ASCII.GetBytes($elToritoID)
[Array]::Copy($idBytes, 0, $isoBuffer, $brvdOff + 7, $idBytes.Length)

# Boot Catalog Pointer (LBA of sector 19)
$catalogLBA = 19
[System.BitConverter]::GetBytes([uint32]$catalogLBA).CopyTo($isoBuffer, $brvdOff + 71)

# =========================================================================
# Sector 18: Volume Descriptor Set Terminator
# =========================================================================
$termOff = 18 * $sectorSize
$isoBuffer[$termOff + 0] = 0xFF # Terminator Type
Set-Ascii $isoBuffer ($termOff + 1) "CD001" 5
$isoBuffer[$termOff + 6] = 0x01

# =========================================================================
# Sector 19: Boot Catalog
# =========================================================================
$catOff = 19 * $sectorSize

# 1. Validation Entry (32 bytes)
$isoBuffer[$catOff + 0] = 0x01 # Header ID
$isoBuffer[$catOff + 1] = 0x00 # Platform: 80x86
$isoBuffer[$catOff + 2] = 0x00 # Reserved
$isoBuffer[$catOff + 3] = 0x00
Set-Ascii $isoBuffer ($catOff + 4) "LohithOS" 24
$isoBuffer[$catOff + 30] = 0x55 # Key 55
$isoBuffer[$catOff + 31] = 0xAA # Key AA

# Compute Validation Entry Checksum (Word sum of 16 words must equal 0)
$sum = 0
for ($w = 0; $w -lt 16; $w++) {
    if ($w -ne 14) { # Skip checksum field itself
        $val = [uint16]($isoBuffer[$catOff + $w * 2] + ($isoBuffer[$catOff + $w * 2 + 1] -shl 8))
        $sum = ($sum + $val) -band 0xFFFF
    }
}
$chk = (0x10000 - $sum) -band 0xFFFF
[System.BitConverter]::GetBytes([uint16]$chk).CopyTo($isoBuffer, $catOff + 28)

# 2. Initial/Default Entry (32 bytes at offset 32)
$entryOff = $catOff + 32
$isoBuffer[$entryOff + 0] = 0x88 # Bootable flag
$isoBuffer[$entryOff + 1] = 0x00 # Boot media: No Emulation
$isoBuffer[$entryOff + 2] = 0x00 # Load Segment: 0 = default (0x07C0)
$isoBuffer[$entryOff + 3] = 0x00
$isoBuffer[$entryOff + 4] = 0x00 # System Type
$isoBuffer[$entryOff + 5] = 0x00 # Unused

# Sector count to load into RAM in 512-byte sectors
# Load stage1 (2048 bytes = 4 sectors) + kernel (~64KB = 128 sectors) = 132 sectors
$count512 = [uint16](4 + ($kernelSectorCount * 4))
if ($count512 -lt 128) { $count512 = 128 }
[System.BitConverter]::GetBytes([uint16]$count512).CopyTo($isoBuffer, $entryOff + 6)

# Load RBA (Starting 2048-byte sector number = 20)
[System.BitConverter]::GetBytes([uint32]$bootSectorLBA).CopyTo($isoBuffer, $entryOff + 8)

# =========================================================================
# Sector 20: Stage 1 Boot Sector (2048 bytes)
# =========================================================================
$stage1Off = $bootSectorLBA * $sectorSize
[Array]::Copy($stage1Bytes, 0, $isoBuffer, $stage1Off, $stage1Bytes.Length)

# =========================================================================
# Sector 21+: Kernel Binary
# =========================================================================
$kernelOff = $kernelLBA * $sectorSize
[Array]::Copy($kernelBytes, 0, $isoBuffer, $kernelOff, $kernelBytes.Length)

# Write complete ISO file to disk
[System.IO.File]::WriteAllBytes($isoPath, $isoBuffer)

$fileInfo = Get-Item $isoPath
Write-Host "Bootable ISO successfully generated: $isoPath" -ForegroundColor Green
Write-Host "Total Size: $($fileInfo.Length) bytes ($([math]::Round($fileInfo.Length / 1KB, 1)) KB)" -ForegroundColor Green
