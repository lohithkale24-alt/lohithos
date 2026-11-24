---
title: LohithOS 32-bit Custom Operating System
emoji: 🖥️
colorFrom: blue
colorTo: indigo
sdk: static
pinned: false
license: mit
---

# LohithOS / MyOS - Bare-Metal Custom Operating System
### An Academic Operating Systems Project Built from Scratch (x86 32-bit Protected Mode)

---

## 1. Project Overview & Abstract
**LohithOS** is a genuine, lightweight, monolithic operating system engineered from the ground up in **C** and **x86 Assembly**. It is **not** a Linux distribution, modified Ubuntu, or based on any existing desktop environment. It implements its own bootloader sequence, CPU interrupt tables (GDT & IDT), programmable interrupt controller (PIC 8259), interval timer (PIT 8254), dynamic heap memory manager, VESA linear framebuffer graphics engine, PS/2 mouse and keyboard hardware drivers, PCI bus enumerator, Intel PRO/1000 MT network card driver, TCP/IP network stack, virtual filesystem (VFS), and a modern graphical desktop interface with interactive retro arcade games and diagnostic tools.

### Key Specifications:
- **Architecture**: 32-bit x86 Protected Mode (`i686-unknown-none-elf` freestanding)
- **Primary Target**: Oracle VirtualBox (VirtualBox 7.x/6.x) & Bare-Metal PC via USB Flash Drive
- **Screen Resolution**: 1024 x 768 @ 32-bit TrueColor Linear Framebuffer (VESA VBE)
- **Rendering Architecture**: High-speed double-buffering (zero screen flicker)
- **Network Interface**: Intel PRO/1000 MT Desktop (82540EM / e1000 PCI)
- **Network Protocols**: Ethernet II, ARP, IPv4, ICMP Echo (`ping`), UDP, DHCP client, DNS resolver
- **Host Wi-Fi Integration**: Seamless NAT bridging through the host's Wi-Fi / Ethernet connection
- **Built-in Games**: Snake Food Bite, Snakes & Ladders, Speed Car 2D Highway Racer
- **Built-in Apps**: Diagnostic Terminal Shell, File Manager, Text Editor, Calculator, System Info, Network Ping Diagnostic Tool
- **Final Image Size**: **126 KB** (ISO) / **4 MB** (USB raw disk) — **100% compliant with the < 100 MB academic limit**!

---

## 2. System Architecture & Flowcharts

### A. Boot & Kernel Initialization Flow
```
+--------------------------------------------------------------------------+
|  BIOS / UEFI CSM (Power-On Self-Test)                                   |
+--------------------------------------------------------------------------+
                                    |
                                    v
+--------------------------------------------------------------------------+
|  Stage-1 El-Torito / MBR Bootloader (Loaded at 0x7C00 in 16-bit Real Mode)|
|  1. Queries VBE BIOS Extensions (INT 0x10, AX=0x4F02 / AX=0x4F01)        |
|  2. Sets 1024x768x32bpp Linear Framebuffer Mode                          |
|  3. Enables Fast A20 Gate (Port 0x92)                                    |
|  4. Loads 32-bit GDT & Switches CPU to Protected Mode (CR0 PE bit = 1)   |
|  5. Relocates Kernel Image to 1MB (0x100000)                             |
|  6. Sets up Multiboot Info at 0x10000 and jumps to 0x100000              |
+--------------------------------------------------------------------------+
                                    |
                                    v
+--------------------------------------------------------------------------+
|  Kernel Entry Point (_start in boot/boot.S -> kmain in kernel/kernel.c)  |
|  1. gdt_init(): Sets up Flat 4GB Kernel/User Code & Data Segments        |
|  2. idt_init() & pic_remap(): Remaps IRQs 0-15 to interrupts 32-47       |
|  3. isr_init(): Registers CPU exception handlers (0-31) and hardware IRQs|
|  4. heap_init(): Configures 8MB dynamic memory heap (kmalloc / kfree)     |
|  5. timer_init(100): Configures 8254 PIT for 100 Hz (10ms tick)          |
|  6. keyboard_init(): Configures PS/2 keyboard scancode decoder (IRQ 1)   |
|  7. gfx_init(): Initializes double-buffered 1024x768 framebuffer canvas |
|  8. mouse_init(): Enables PS/2 streaming mouse with cursor drawing(IRQ 12)|
|  9. sti(): Enables hardware CPU interrupts                               |
|  10. pci_scan(): Scans PCI bus for Intel PRO/1000 MT (82540EM) NIC       |
|  11. e1000_init(): Sets up RX/TX ring descriptors & reads hardware MAC   |
|  12. net_init(): Initializes Ethernet, ARP, IPv4, ICMP, DHCP, DNS stack   |
|  13. vfs_init(): Preloads in-memory Virtual Filesystem hierarchy         |
|  14. gui_init(): Launches Modern Desktop GUI with Windows & Start Menu   |
+--------------------------------------------------------------------------+
                                    |
                                    v
+--------------------------------------------------------------------------+
|  Real-Time Desktop Event & Compositing Loop                              |
|  - net_poll(): Processes incoming Ethernet & ICMP ping packets           |
|  - kbd_poll(): Routes keystrokes to the focused window                   |
|  - gui_handle_mouse(): Window dragging, button clicks, icon launching     |
|  - gui_render_all(): Composites wallpaper, windows, taskbar & cursor     |
|  - gfx_flip(): Blits backbuffer to VRAM during screen refresh             |
+--------------------------------------------------------------------------+
```

### B. Physical Memory Layout
```
0x00000000 +-------------------------------------------------------+
           | Real-Mode Interrupt Vector Table (IVT)                |
0x00000400 +-------------------------------------------------------+
           | BIOS Data Area (BDA)                                  |
0x00007C00 +-------------------------------------------------------+
           | Stage-1 Boot Sector (2048 bytes)                      |
0x00010000 +-------------------------------------------------------+
           | Multiboot Information Structure (VBE Framebuffer info)|
0x00080000 +-------------------------------------------------------+
           | Kernel Execution Stack (32 KB top at 0x90000)         |
0x00100000 +-------------------------------------------------------+
  (1 MB)   | Kernel Binary Code (.text, .rodata, .data, .bss)      |
0x00400000 +-------------------------------------------------------+
  (4 MB)   | Kernel Dynamic Heap (8 MB heap: 0x00400000 - 0x00C00000)
0xE0000000 +-------------------------------------------------------+
 (3.5 GB+) | VESA VBE Linear Framebuffer VRAM (1024x768x4 = 3 MB)  |
+-------------------------------------------------------------------+
```

---

## 3. Directory & File Structure
```
c:\Users\DELL\Desktop\lohithos/
├── boot/
│   ├── boot.S            # Multiboot header & 32-bit kernel startup entry
│   ├── interrupts.S      # Low-level CPU exception (0..31) & hardware IRQ stubs
│   ├── stage1_iso.S      # Direct BIOS El-Torito bootloader with VBE mode switch
│   └── linker.ld         # Freestanding kernel linker script (loads at 1MB)
├── kernel/
│   ├── kernel.c          # Master kernel entry point and compositing loop
│   ├── gdt.c             # Global Descriptor Table initialization
│   ├── idt.c             # Interrupt Descriptor Table initialization
│   ├── pic.c             # 8259 Programmable Interrupt Controller remapping
│   ├── isr.c             # Interrupt service routine dispatcher
│   ├── timer.c           # 8254 Programmable Interval Timer (100 Hz ticks)
│   ├── heap.c            # Dynamic memory allocator (kmalloc / kfree)
│   ├── net.c             # TCP/IP network stack (Ethernet, ARP, IPv4, ICMP)
│   ├── fs.c              # In-memory Virtual Filesystem (VFS)
│   ├── gui.c             # Modern Window Manager, Desktop, Taskbar & Start Menu
│   └── string.c          # Freestanding C runtime (memset, memcpy, sprintf, etc.)
├── drivers/
│   ├── font.c            # 8x16 bitmap font definition table
│   ├── graphics.c        # Framebuffer primitives (double-buffering, lines, rects)
│   ├── keyboard.c        # PS/2 keyboard driver with Scancode Set 1 decoding
│   ├── mouse.c           # PS/2 mouse driver with packet decoding & cursor blit
│   ├── pci.c             # PCI bus scanner and BAR configuration
│   └── e1000.c           # Intel PRO/1000 MT (82540EM) NIC driver
├── apps/
│   ├── apps.c            # Terminal, File Manager, Editor, Calc, SysInfo, Ping
│   └── games.c           # Snake Food Bite, Snakes & Ladders, Speed Car Racer
├── include/              # Comprehensive C header definitions
│   ├── types.h, multiboot.h, gdt.h, idt.h, pic.h, isr.h, timer.h, heap.h
│   ├── graphics.h, font.h, keyboard.h, mouse.h, pci.h, e1000.h, net.h
│   └── fs.h, gui.h, apps.h, games.h
├── iso/                  # GRUB directory structure for Linux builds
│   └── boot/grub/grub.cfg
├── build.ps1             # Native Windows build script using LLVM Clang & LLD
├── build_iso.ps1         # Standalone El-Torito bootable ISO generator
├── build_usb.ps1         # Bootable USB flash drive raw disk image creator
├── setup_vbox.ps1        # Automated Oracle VirtualBox VM creator
├── Makefile              # Ubuntu WSL2 / Linux GCC build script
└── PROJECT_STATUS.md     # Engineering tracker for all features and tests
```

---

## 4. How to Build & Run

### Method 1: On Windows 10 (Using Native LLVM Clang - No WSL Required)
1. Open Windows PowerShell in this folder:
   ```powershell
   cd C:\Users\DELL\Desktop\lohithos
   ```
2. Build the kernel binary and bootable ISO:
   ```powershell
   powershell -ExecutionPolicy Bypass -File .\build.ps1
   ```
3. Create and register the VirtualBox VM:
   ```powershell
   powershell -ExecutionPolicy Bypass -File .\setup_vbox.ps1
   ```
4. Start the VM:
   - Either open **Oracle VM VirtualBox Manager** from the Windows Start menu and click **Start** on `LohithOS`.
   - Or launch it directly from PowerShell:
     ```powershell
     & "C:\Program Files\Oracle\VirtualBox\VBoxManage.exe" startvm "LohithOS"
     ```

### Method 2: Inside Ubuntu WSL2 (Using GCC & GRUB)
If you copy the project directory to Ubuntu WSL2 (`/home/rishika_v/myos` or any Ubuntu directory):
1. Build with `make`:
   ```bash
   make
   ```
2. This creates `kernel.bin` and `lohithos.iso` using `grub-mkrescue` and `xorriso`.

---

## 5. How to Create a Bootable Physical USB Pendrive Safely
> [!WARNING]
> Writing an operating system image to a physical USB drive **erases all data on that USB drive**. Always double-check your drive selection and back up important files before proceeding. Never write to your computer's internal hard drive!

### Using Rufus on Windows (Recommended):
1. Insert your USB pendrive into your PC.
2. Download and run **Rufus** (https://rufus.ie).
3. Under **Device**, select your USB flash drive.
4. Under **Boot selection**, click **SELECT** and browse to `C:\Users\DELL\Desktop\lohithos\lohithos.iso` (or `lohithos_usb.img`).
5. Click **START**. If prompted between ISO mode and DD Image mode, choose **Write in DD Image mode**.
6. When flashing completes, insert the USB into your laptop/PC, power on, and press your boot menu key (**F12**, **F11**, or **F8**) to boot directly into LohithOS!

---

## 6. Built-in Applications & Interactive Games

### Desktop Applications:
1. **Diagnostic Terminal Shell**:
   - Interactive CLI shell with prompt `lohithos> `
   - Supported commands: `help`, `clear`, `echo`, `ip` / `ifconfig`, `ping <ip>`, `wifi`, `ls`, `cat <file>`, `mem`, `pci`, `uptime`, `reboot`.
2. **System Information (My PC)**:
   - Displays real-time kernel telemetry, CPU architecture, heap usage, VBE resolution, MAC address, and IP configuration.
3. **Visual Calculator**:
   - Full graphical arithmetic calculator (+, -, *, /).
4. **Text Editor (Notepad)**:
   - Scratchpad viewer and editor with a functional **SAVE** button connected to the Virtual Filesystem.
5. **File Manager**:
   - Visual directory browser for `/docs/`, `/system/`, and `/games/` files. Click any file to open it in the Text Editor.
6. **Network Ping Diagnostics**:
   - Dedicated GUI utility to test round-trip ICMP latency to the Gateway (`10.0.2.2`), DNS (`10.0.2.3`), Google (`8.8.8.8`), and Cloudflare (`1.1.1.1`).

### Retro Arcade Games:
1. **Snake Food Bite**:
   - Classic Snake game with grid movement, food generation, score counter, and collision detection. Steer with Arrow Keys; press Space to restart.
2. **Snakes & Ladders**:
   - Interactive board game with a 6x6 board (tiles 1 to 36). Click **ROLL DICE** to roll 1..6, climb green ladders (4->14, 9->22, 18->29), and avoid red snakes (17->7, 25->10, 32->15).
3. **Speed Car 2D Racer**:
   - Retro arcade highway racer. Dodge oncoming traffic using Left/Right arrow keys. Score increments with every avoided vehicle!

---

## 7. College Project Verification & Defense Questions

| Question | Answer for Academic Evaluators |
| :--- | :--- |
| **Is this based on Linux?** | **No.** There is zero Linux kernel code. The kernel, scheduler, memory manager, drivers, and GUI are 100% custom-written in freestanding C and x86 Assembly. |
| **How does graphics work without an OS?** | During boot, the bootloader invokes BIOS INT 0x10 (VBE mode `0x4118`) to request a 1024x768x32 linear framebuffer. The kernel writes 32-bit ARGB pixels directly to physical VRAM using high-performance double-buffering. |
| **How does networking work?** | The kernel scans the PCI bus for device `8086:100E` (Intel PRO/1000 MT), maps its MMIO registers, initializes DMA ring buffers, and implements custom Ethernet II, ARP, IPv4, ICMP Echo (`ping`), and UDP packet parsers. |
| **How is Wi-Fi supported?** | VirtualBox does not emulate physical 802.11 Wi-Fi chips; instead, VirtualBox's virtual Intel e1000 Ethernet adapter bridges through the host's Wi-Fi network seamlessly via NAT, providing real external Internet access. |
| **What is the image size?** | The entire bootable ISO is **126 KB**, which easily meets the college requirement of being under 100 MB. |
