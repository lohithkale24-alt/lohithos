#include "types.h"
#include "multiboot.h"
#include "gdt.h"
#include "idt.h"
#include "pic.h"
#include "isr.h"
#include "timer.h"
#include "heap.h"
#include "graphics.h"
#include "keyboard.h"
#include "mouse.h"
#include "pci.h"
#include "e1000.h"
#include "net.h"
#include "fs.h"
#include "gui.h"
#include "apps.h"
#include "games.h"

void kmain(uint32_t magic, multiboot_info_t *mbi) {
    (void)magic;

    /* 1. Global Descriptor Table */
    gdt_init();

    /* 2. Interrupt Descriptor Table & 8259 PIC Remap */
    idt_init();
    pic_remap(32, 40);
    isr_init();

    /* 3. Kernel Dynamic Memory Heap (8 MB heap at 4MB) */
    heap_init(0x00400000, 8 * 1024 * 1024);

    /* 4. Programmable Interval Timer (100 Hz = 10ms tick) */
    timer_init(100);

    /* 5. PS/2 Keyboard Driver */
    keyboard_init();

    /* 6. VBE High-Resolution Linear Framebuffer Graphics */
    gfx_init(mbi);

    /* 7. PS/2 Mouse Driver with Boundary Clamping */
    mouse_init(g_fb.width, g_fb.height);

    /* 8. Enable CPU Interrupts */
    sti();

    /* 9. PCI Bus Enumeration */
    pci_scan();

    /* 10. Intel PRO/1000 MT (82540EM) Network Card Driver */
    pci_device_t *nic_dev = pci_find_device(E1000_VENDOR_INTEL, E1000_DEV_82540EM);
    if (!nic_dev) nic_dev = pci_find_device(E1000_VENDOR_INTEL, E1000_DEV_82545EM);
    if (!nic_dev) nic_dev = pci_find_device(E1000_VENDOR_INTEL, E1000_DEV_82543GC);

    if (nic_dev) {
        e1000_init(nic_dev);
    }

    /* 11. TCP/IP Network Stack (Ethernet, ARP, IPv4, ICMP Ping, UDP) */
    net_init();

    /* 12. In-Memory Virtual File System (VFS) */
    vfs_init();

    /* 13. Modern Graphical Desktop Environment */
    gui_init();

    /* Open default startup windows */
    app_sysinfo_open();
    app_terminal_open();

    /* 14. Real-time Desktop Compositor & Event Loop */
    while (1) {
        /* Poll incoming network frames */
        net_poll();

        /* Poll keyboard events */
        if (kbd_has_key()) {
            char k = kbd_getchar();
            gui_handle_keyboard(k);
        }

        /* Poll mouse events & window interactions */
        gui_handle_mouse();

        /* Composite and refresh frame */
        gui_render_all();

        /* Low power wait */
        hlt();
    }
}
