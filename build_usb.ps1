# build_usb.ps1 - Bootable USB Flash Drive Image Creator for LohithOS
# Creates a raw bootable disk image (lohithos_usb.img) for flashing to a physical USB drive
$ErrorActionPreference = "Stop"

$usbImg = "lohithos_usb.img"
$stage1Path = "boot/stage1_iso.bin"
$kernelPath = "kernel.bin"

Write-Host "=========================================" -ForegroundColor Cyan
Write-Host "  Generating Bootable USB Image: $usbImg " -ForegroundColor Cyan
Write-Host "=========================================" -ForegroundColor Cyan

$stage1Bytes = [System.IO.File]::ReadAllBytes($stage1Path)
$kernelBytes = [System.IO.File]::ReadAllBytes($kernelPath)

# 4 MB disk image size
$diskSize = 4 * 1024 * 1024
$diskBuffer = New-Object byte[] $diskSize

# Sector 0: MBR / Stage 1 Boot Sector (512 bytes)
[Array]::Copy($stage1Bytes, 0, $diskBuffer, 0, 512)

# Ensure MBR Boot Signature 0x55AA at bytes 510-511
$diskBuffer[510] = 0x55
$diskBuffer[511] = 0xAA

# Kernel sectors start at Sector 1 (offset 512)
[Array]::Copy($kernelBytes, 0, $diskBuffer, 512, $kernelBytes.Length)

[System.IO.File]::WriteAllBytes($usbImg, $diskBuffer)

$imgInfo = Get-Item $usbImg
Write-Host "USB Image created: $usbImg ($([math]::Round($imgInfo.Length / 1MB, 2)) MB)" -ForegroundColor Green

Write-Host "`n-----------------------------------------------------------" -ForegroundColor Yellow
Write-Host "  HOW TO CREATE A BOOTABLE PHYSICAL USB PENDRIVE SAFELY    " -ForegroundColor Yellow
Write-Host "-----------------------------------------------------------" -ForegroundColor Yellow
Write-Host "WARNING: Writing to a physical USB will ERASE all data on that USB." -ForegroundColor Red
Write-Host "Make sure you select the correct USB drive letter and backup important files." -ForegroundColor Red
Write-Host "`nOption A (Recommended on Windows): Use Rufus" -ForegroundColor Cyan
Write-Host "  1. Download and run Rufus (https://rufus.ie)"
Write-Host "  2. Under 'Device', select your target USB Pendrive."
Write-Host "  3. Under 'Boot selection', click 'SELECT' and choose either 'lohithos.iso' or 'lohithos_usb.img'."
Write-Host "  4. Click 'START'. When prompted, select 'Write in DD Image mode'."
Write-Host "  5. Insert the USB into any PC and boot from USB via BIOS boot menu (F12 / F11 / F8)."
Write-Host "`nOption B: In Ubuntu / Linux terminal" -ForegroundColor Cyan
Write-Host "  sudo dd if=lohithos.iso of=/dev/sdX bs=4M status=progress && sync"
Write-Host "  (Replace /dev/sdX with your actual USB device, e.g., /dev/sdb - DO NOT use /dev/sda!)"
Write-Host "===========================================================" -ForegroundColor Green
