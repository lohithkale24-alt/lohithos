/* stage1_iso.S - Direct BIOS El-Torito Stage-1 Bootloader
 * Boots directly from ISO 9660 / USB, sets 1024x768 VBE framebuffer,
 * switches to 32-bit protected mode, sets up Multiboot structure,
 * and executes kmain.
 */
.code16
.section .text
.global _start_stage1
_start_stage1:
    cli
    xor %ax, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %ss
    mov $0x7C00, %sp
    sti

    mov %dl, (boot_drive)

    /* Query and Set VBE 1024x768x32 linear framebuffer (Mode 0x4118 or 0x411B or 0x4115) */
    /* VirtualBox supports 0x4118 (1024x768x32) or 0x4115 (800x600x32) */
    mov $0x4F02, %ax
    mov $0x4118, %bx      /* 1024x768x32 LFB */
    int $0x10
    cmp $0x004F, %ax
    je .vbe_ok

    /* Fallback to 800x600x32 if 1024x768 fails */
    mov $0x4F02, %ax
    mov $0x4115, %bx
    int $0x10
    cmp $0x004F, %ax
    je .vbe_ok

    /* Fallback to 1024x768x24 */
    mov $0x4F02, %ax
    mov $0x4117, %bx
    int $0x10

.vbe_ok:
    /* Get VBE Mode Info to retrieve physical linear framebuffer address */
    mov $0x4F01, %ax
    mov $0x4118, %cx
    mov $0x9000, %di      /* buffer at 0x9000:0x0000 -> 0x90000 */
    mov %di, %es
    xor %di, %di
    int $0x10

    /* Physical framebuffer base address is at offset 40 (0x28) of VBE Mode Info */
    mov %es:0x28, %eax
    test %eax, %eax
    jnz .lfb_found
    mov $0xE0000000, %eax /* Standard VirtualBox VRAM fallback */
.lfb_found:
    mov %eax, (fb_phys_addr)

    /* Enable Fast A20 Gate */
    inb $0x92, %al
    orb $0x02, %al
    outb %al, $0x92

    /* Disable interrupts before entering Protected Mode */
    cli

    /* Load Global Descriptor Table */
    lgdt (gdt_descriptor)

    /* Set PE bit in CR0 */
    mov %cr0, %eax
    or $1, %eax
    mov %eax, %cr0

    /* Far jump to 32-bit Protected Mode */
    ljmp $0x08, $protected_mode

.code32
protected_mode:
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss
    mov $0x90000, %esp

    /* Copy kernel image from load address (0x8000) to 1MB (0x100000) */
    /* Stage 1 is 2048 bytes (0x7C00..0x83FF). Kernel starts at 0x8400. */
    mov $0x8400, %esi
    mov $0x100000, %edi
    mov $32768, %ecx      /* Copy 128 KB (32768 DWORDs) */
    cld
    rep movsl

    /* Prepare Multiboot Info structure at 0x10000 */
    mov $0x10000, %edi
    mov $0, %eax
    mov $32, %ecx
    rep stosl             /* Zero out multiboot structure */

    /* Set flags: MULTIBOOT_INFO_FRAMEBUFFER (1 << 12) | MULTIBOOT_INFO_MEMORY (1 << 0) */
    movl $0x1001, 0x10000

    /* Memory upper (64MB) */
    movl $65536, 0x10008

    /* Framebuffer info at offset 88 (0x58) */
    mov (fb_phys_addr), %eax
    mov %eax, 0x10058     /* framebuffer_addr (64-bit low) */
    movl $0,  0x1005C     /* framebuffer_addr (64-bit high) */
    movl $4096, 0x10060   /* framebuffer_pitch */
    movl $1024, 0x10064   /* framebuffer_width */
    movl $768,  0x10068   /* framebuffer_height */
    movb $32,   0x1006C   /* framebuffer_bpp */
    movb $1,    0x1006D   /* framebuffer_type (RGB) */

    /* Set Multiboot register contract: %eax = 0x2BADB002, %ebx = &mbi */
    mov $0x2BADB002, %eax
    mov $0x10000, %ebx

    /* Jump to kernel entry point at 1MB */
    jmp 0x101000

/* 16-bit real mode data */
.code16
boot_drive:   .byte 0
fb_phys_addr: .long 0xE0000000

.align 4
gdt_start:
    /* Null descriptor */
    .quad 0
    /* Kernel 32-bit Code Segment: base=0, limit=4GB, ring 0, rx */
    .word 0xFFFF, 0x0000
    .byte 0x00, 0x9A, 0xCF, 0x00
    /* Kernel 32-bit Data Segment: base=0, limit=4GB, ring 0, rw */
    .word 0xFFFF, 0x0000
    .byte 0x00, 0x92, 0xCF, 0x00
gdt_end:

gdt_descriptor:
    .word gdt_end - gdt_start - 1
    .long gdt_start

/* Pad stage1 to exactly 2048 bytes (1 CD sector) */
. = _start_stage1 + 2048
