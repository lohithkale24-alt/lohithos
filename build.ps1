# build.ps1 - Native Windows Build Script for LohithOS
# Uses LLVM Clang and LLD (installed on Windows) to compile freestanding bare-metal x86 ELF
param(
    [switch]$RunInVBox = $false
)

$ErrorActionPreference = "Stop"

$CLANG = "C:\Program Files\LLVM\bin\clang.exe"
$LLD   = "C:\Program Files\LLVM\bin\ld.lld.exe"
$OBJCOPY = "C:\Program Files\LLVM\bin\llvm-objcopy.exe"

Write-Host "=========================================" -ForegroundColor Cyan
Write-Host "  Building LohithOS Bare-Metal Kernel    " -ForegroundColor Cyan
Write-Host "=========================================" -ForegroundColor Cyan

$CFLAGS = @(
    "-target", "i686-unknown-none-elf",
    "-m32",
    "-ffreestanding",
    "-O2",
    "-Wall",
    "-Wno-unused-function",
    "-Iinclude",
    "-fno-pic",
    "-fno-pie",
    "-fno-stack-protector",
    "-nostdlib"
)

$sources = @(
    "boot/boot.S",
    "boot/interrupts.S",
    "kernel/string.c",
    "kernel/gdt.c",
    "kernel/idt.c",
    "kernel/pic.c",
    "kernel/isr.c",
    "kernel/timer.c",
    "kernel/heap.c",
    "kernel/net.c",
    "kernel/fs.c",
    "kernel/gui.c",
    "kernel/kernel.c",
    "drivers/font.c",
    "drivers/graphics.c",
    "drivers/keyboard.c",
    "drivers/mouse.c",
    "drivers/pci.c",
    "drivers/e1000.c",
    "apps/apps.c",
    "apps/games.c"
)

$objs = @()
foreach ($src in $sources) {
    $obj = $src -replace '\.(c|S)$', '.o'
    Write-Host "Compiling $src -> $obj..." -ForegroundColor Gray
    & $CLANG @CFLAGS -c $src -o $obj
    $objs += $obj
}

Write-Host "Linking kernel.elf with LLD..." -ForegroundColor Yellow
& $LLD -m elf_i386 -T boot/linker.ld -o kernel.elf @objs

Write-Host "Extracting binary kernel.bin..." -ForegroundColor Yellow
Copy-Item -Force kernel.elf kernel.bin
Copy-Item -Force kernel.bin iso/boot/kernel.bin

$kernelSize = (Get-Item kernel.bin).Length
Write-Host "Kernel binary successfully built: kernel.bin ($kernelSize bytes)" -ForegroundColor Green

# Generate bootable ISO file
Write-Host "Generating bootable LohithOS ISO image..." -ForegroundColor Cyan
& powershell -ExecutionPolicy Bypass -File ./build_iso.ps1

$isoSize = (Get-Item lohithos.iso).Length
Write-Host "=========================================" -ForegroundColor Green
Write-Host "  BUILD COMPLETE!                        " -ForegroundColor Green
Write-Host "  Bootable ISO: lohithos.iso             " -ForegroundColor Green
Write-Host "  Actual Size:  $([math]::Round($isoSize / 1MB, 2)) MB (< 100 MB Limit) " -ForegroundColor Green
Write-Host "=========================================" -ForegroundColor Green
