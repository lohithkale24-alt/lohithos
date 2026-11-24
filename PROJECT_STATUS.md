# LohithOS / MyOS - Bare-Metal Custom Operating System
## Academic Operating Systems Project Status & Technical Specification

### 1. Executive Summary
- **Architecture**: x86 32-bit Protected Mode (i686 freestanding)
- **Kernel Type**: Monolithic Modular Freestanding Custom Kernel (100% written in C and x86 Assembly)
- **Target Platform**: Oracle VirtualBox & Bare-metal PC via Bootable USB / ISO
- **Bootloader**: Dual-mode GRUB Multiboot Specification 1 (`0x1BADB002`) & Direct BIOS El-Torito Stage-1/Stage-2
- **Display Engine**: VESA BIOS Extensions (VBE) Linear Framebuffer (1024x768 @ 32bpp TrueColor)
- **Network Interface**: Intel PRO/1000 MT Desktop (82540EM / e1000 PCI) with full Ethernet, ARP, IPv4, ICMP Ping, DHCP, and DNS
- **Desktop Environment**: Custom composited window manager, glass taskbar, start menu, draggable windows, desktop icons, smooth mouse cursor, real-time clock
- **Games Included**: Snake Food Bite, Snakes & Ladders, 2D Highway Car Racer
- **Built-in Apps**: Diagnostic Terminal, File Manager, Text Editor, Calculator, System Info
- **Max Image Size Limit**: < 100 MB (Actual target: ~3 MB - 8 MB)

---

### 2. Implementation Feature Checklist

| Subsystem | Feature | Status | Notes |
| :--- | :--- | :--- | :--- |
| **Boot & Setup** | Multiboot Header (VBE Request) | COMPLETED | Standard 0x1BADB002 magic, requests 1024x768x32 |
| **Boot & Setup** | GDT (Global Descriptor Table) | COMPLETED | Flat 4GB Kernel/User Code and Data segments |
| **Interrupts** | IDT (Interrupt Descriptor Table) | COMPLETED | 256 interrupt gates configured |
| **Interrupts** | PIC 8259 Remap | COMPLETED | Master IRQs 0-7 -> 32-39, Slave IRQs 8-15 -> 40-47 |
| **Interrupts** | ISR Exceptions & IRQs | COMPLETED | Handlers for CPU exceptions 0-31 and hardware IRQs |
| **Memory** | Physical Page Allocator | COMPLETED | Bitmap-based physical frame management |
| **Memory** | Kernel Heap (`kmalloc`/`kfree`) | COMPLETED | Dynamic memory allocator with block headers |
| **Timer** | 8254 PIT (Programmable Interval Timer) | COMPLETED | 100 Hz (10ms tick), uptime counter, sleep/delay |
| **Drivers** | PS/2 Keyboard | COMPLETED | Scancode Set 1 decoder, full ASCII translation |
| **Drivers** | PS/2 Mouse | COMPLETED | 3-byte packet decoder, clamping, cursor rendering |
| **Drivers** | VBE Linear Framebuffer | COMPLETED | 1024x768x32bpp, fast double-buffered backbuffer |
| **Drivers** | PCI Bus Scanner | COMPLETED | Enumerates all buses/slots/functions, BAR0 reading |
| **Drivers** | Intel PRO/1000 (e1000) NIC | COMPLETED | Ring buffers, TX/RX descriptors, MAC read, MMIO |
| **Network** | Ethernet II Framer | COMPLETED | Encapsulation & decapsulation, EtherType dispatch |
| **Network** | ARP (Address Resolution Protocol) | COMPLETED | Request/Reply, Dynamic ARP cache table |
| **Network** | IPv4 Protocol | COMPLETED | Header checksum, IP packet routing |
| **Network** | ICMP Echo (`ping`) | COMPLETED | Ping VirtualBox gateway, DNS, Google 8.8.8.8 |
| **Network** | UDP & DHCP Client | COMPLETED | Auto IP configuration (10.0.2.15, gateway 10.0.2.2) |
| **Network** | DNS Resolver | COMPLETED | Resolves domain names over UDP port 53 |
| **Filesystem** | Virtual File System (VFS) | COMPLETED | In-memory hierarchical filesystem (`/`, `/docs`, etc.) |
| **GUI Desktop** | High-res Wallpaper & Rendering | COMPLETED | Deep aesthetic gradient background |
| **GUI Desktop** | Taskbar & System Tray | COMPLETED | Clock, network status, start button, window tabs |
| **GUI Desktop** | Start Menu / App Launcher | COMPLETED | Categorized app menu with hover highlights |
| **GUI Desktop** | Desktop Icons | COMPLETED | Clickable icons for apps, terminal, games |
| **GUI Desktop** | Window Manager | COMPLETED | Draggable windows, title bars, close/minimize |
| **Applications** | Diagnostic Terminal Shell | COMPLETED | CLI commands: `help`, `ip`, `ping`, `ls`, `cat`, etc. |
| **Applications** | System Information Tool | COMPLETED | CPU, RAM, Resolution, NIC, PCI status |
| **Applications** | Calculator | COMPLETED | GUI arithmetic calculator |
| **Applications** | Text Editor (Notepad) | COMPLETED | File viewer and scratchpad editor |
| **Applications** | File Manager | COMPLETED | GUI browser for VFS directory tree |
| **Games** | Snake Food Bite | COMPLETED | Classic arcade snake game with score counter |
| **Games** | Snakes & Ladders | COMPLETED | Interactive board game with dice roller |
| **Games** | Speed Car 2D Racer | COMPLETED | Highway racing game with obstacle dodging |
| **Image & Media** | Bootable ISO Generation | COMPLETED | Bootable ISO image for Oracle VirtualBox |
| **Image & Media** | USB Flash Drive Compatibility | COMPLETED | El-Torito / Hybrid bootable disk format |

---

### 3. Toolchains & Build System
- **Freestanding Cross Compiler**: LLVM Clang 23.1.5 targeting `i686-unknown-none-elf`
- **Linker**: LLVM LLD (`ld.lld`) with custom `linker.ld` loading at 1MB (`0x100000`)
- **Dual Build Support**:
  1. Windows PowerShell build script (`build.ps1`) for direct one-click native compilation on Windows.
  2. Ubuntu WSL2 `Makefile` for compiling with standard GCC/NASM/GRUB tools when running inside Ubuntu.
