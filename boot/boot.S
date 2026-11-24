/* Multiboot Header & 32-bit Protected Mode Kernel Entry Point */
.set ALIGN,    1<<0             /* align loaded modules on page boundaries */
.set MEMINFO,  1<<1             /* provide memory map */
.set VIDMODE,  1<<2             /* request video mode */
.set FLAGS,    ALIGN | MEMINFO | VIDMODE
.set MAGIC,    0x1BADB002       /* 'magic number' lets bootloader find the header */
.set CHECKSUM, -(MAGIC + FLAGS) /* checksum of above, to prove we are multiboot */

.section .multiboot, "a"
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM

/* Header address fields (not needed if not AOUT kludge, but set to 0) */
.long 0
.long 0
.long 0
.long 0
.long 0

/* Video mode preferences */
.long 0       /* Mode type: 0 = linear graphics */
.long 1024    /* Preferred width */
.long 768     /* Preferred height */
.long 32      /* Preferred depth (32-bit truecolor) */

.section .bss
.align 16
stack_bottom:
.skip 32768   /* 32 KB kernel stack */
stack_top:

.section .text
.global _start
.type _start, @function
_start:
    /* Disable interrupts immediately */
    cli

    /* Set up stack pointer */
    mov $stack_top, %esp
    mov %esp, %ebp

    /* Reset EFLAGS */
    pushl $0
    popf

    /* Push arguments for kmain(uint32_t magic, multiboot_info_t *mbi) */
    push %ebx  /* Multiboot info structure pointer */
    push %eax  /* Multiboot magic number */

    /* Call the C kernel entry point */
    .extern kmain
    call kmain

    /* If kmain returns, halt CPU */
.global halt_loop
halt_loop:
    cli
    hlt
    jmp halt_loop
