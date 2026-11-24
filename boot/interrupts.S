/* Low-Level Interrupt Service Routines (ISRs) & IRQs */
.section .text

.macro ISR_NOERRCODE num
.global isr\num
isr\num:
    pushl $0            /* Dummy error code */
    pushl $\num         /* Interrupt number */
    jmp isr_common_stub
.endm

.macro ISR_ERRCODE num
.global isr\num
isr\num:
    pushl $\num         /* Interrupt number (error code already pushed by CPU) */
    jmp isr_common_stub
.endm

.macro IRQ num, irq_num
.global irq\num
irq\num:
    pushl $0
    pushl $\irq_num
    jmp irq_common_stub
.endm

/* CPU Exceptions 0..31 */
ISR_NOERRCODE 0
ISR_NOERRCODE 1
ISR_NOERRCODE 2
ISR_NOERRCODE 3
ISR_NOERRCODE 4
ISR_NOERRCODE 5
ISR_NOERRCODE 6
ISR_NOERRCODE 7
ISR_ERRCODE   8
ISR_NOERRCODE 9
ISR_ERRCODE   10
ISR_ERRCODE   11
ISR_ERRCODE   12
ISR_ERRCODE   13
ISR_ERRCODE   14
ISR_NOERRCODE 15
ISR_NOERRCODE 16
ISR_ERRCODE   17
ISR_NOERRCODE 18
ISR_NOERRCODE 19
ISR_NOERRCODE 20
ISR_NOERRCODE 21
ISR_NOERRCODE 22
ISR_NOERRCODE 23
ISR_NOERRCODE 24
ISR_NOERRCODE 25
ISR_NOERRCODE 26
ISR_NOERRCODE 27
ISR_NOERRCODE 28
ISR_NOERRCODE 29
ISR_ERRCODE   30
ISR_NOERRCODE 31

/* Hardware IRQs 0..15 (mapped to 32..47) */
IRQ 0, 32
IRQ 1, 33
IRQ 2, 34
IRQ 3, 35
IRQ 4, 36
IRQ 5, 37
IRQ 6, 38
IRQ 7, 39
IRQ 8, 40
IRQ 9, 41
IRQ 10, 42
IRQ 11, 43
IRQ 12, 44
IRQ 13, 45
IRQ 14, 46
IRQ 15, 47

.extern isr_handler
isr_common_stub:
    pusha               /* Pushes edi, esi, ebp, esp, ebx, edx, ecx, eax */
    mov %ds, %ax
    pushl %eax          /* Save data segment selector */
    mov $0x10, %ax      /* Load kernel data segment */
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs

    pushl %esp          /* Pass registers_t pointer to isr_handler */
    call isr_handler
    addl $4, %esp

    popl %eax           /* Restore original data segment */
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    popa                /* Restore general registers */
    addl $8, %esp       /* Pop error code and interrupt number */
    iret

.extern irq_handler
irq_common_stub:
    pusha
    mov %ds, %ax
    pushl %eax
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs

    pushl %esp
    call irq_handler
    addl $4, %esp

    popl %eax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    popa
    addl $8, %esp
    iret

.global gdt_flush
.extern g_gdt_ptr
gdt_flush:
    lgdt (g_gdt_ptr)
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss
    ljmp $0x08, $.flush
.flush:
    ret

.global idt_flush
.extern g_idt_ptr
idt_flush:
    lidt (g_idt_ptr)
    ret
