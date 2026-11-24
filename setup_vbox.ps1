# setup_vbox.ps1 - Automated Oracle VirtualBox Configuration for LohithOS
# Configures a high-performance 32-bit VM matching all hardware specifications
param(
    [switch]$StartVM = $false
)

$ErrorActionPreference = "Stop"

$vbox = "C:\Program Files\Oracle\VirtualBox\VBoxManage.exe"
if (-not (Test-Path $vbox)) {
    throw "VirtualBox not found at $vbox"
}

$vmName = "LohithOS"
$isoPath = (Resolve-Path "lohithos.iso").Path

Write-Host "=========================================" -ForegroundColor Cyan
Write-Host "  VirtualBox VM Configuration: $vmName   " -ForegroundColor Cyan
Write-Host "=========================================" -ForegroundColor Cyan

# Check if VM already exists
$existingVMs = & $vbox list vms
if ($existingVMs -match """$vmName""") {
    Write-Host "Existing VM '$vmName' found. Re-registering with latest ISO..." -ForegroundColor Yellow
    & $vbox controlvm $vmName poweroff 2>$null
    Start-Sleep -Seconds 1
    & $vbox unregistervm $vmName --delete 2>$null
}

# 1. Create VM
Write-Host "1. Creating VM definition..." -ForegroundColor Gray
& $vbox createvm --name $vmName --ostype "Other" --register

# 2. Configure Memory, CPU, and Graphics
Write-Host "2. Configuring Hardware: 256MB RAM, 32MB VRAM, VBoxVGA..." -ForegroundColor Gray
& $vbox modifyvm $vmName --memory 256 --vram 32 --graphicscontroller vboxvga --boot1 dvd --boot2 none --mouse ps2 --keyboard ps2

# 3. Configure Intel PRO/1000 MT Desktop NIC (e1000) with NAT / Host Wi-Fi bridge
Write-Host "3. Configuring Intel PRO/1000 MT Desktop Network Adapter..." -ForegroundColor Gray
& $vbox modifyvm $vmName --nic1 nat --nictype1 82540EM --cableconnected1 on

# 4. Add Storage Controller and Attach ISO
Write-Host "4. Attaching LohithOS Bootable ISO ($isoPath)..." -ForegroundColor Gray
& $vbox storagectl $vmName --name "IDE Controller" --add ide --controller PIIX4
& $vbox storageattach $vmName --storagectl "IDE Controller" --port 0 --device 0 --type dvddrive --medium $isoPath

Write-Host "=========================================" -ForegroundColor Green
Write-Host "  VM Successfully Created in VirtualBox! " -ForegroundColor Green
Write-Host "  VM Name: $vmName                       " -ForegroundColor Green
Write-Host "  Attached Media: $isoPath               " -ForegroundColor Green
Write-Host "=========================================" -ForegroundColor Green

if ($StartVM) {
    Write-Host "Launching $vmName in Oracle VirtualBox..." -ForegroundColor Cyan
    & $vbox startvm $vmName
} else {
    Write-Host "To launch the VM, open Oracle VirtualBox and click 'Start', or run:" -ForegroundColor Yellow
    Write-Host "  & 'C:\Program Files\Oracle\VirtualBox\VBoxManage.exe' startvm '$vmName'" -ForegroundColor White
}
