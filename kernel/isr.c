#include "isr.h"
#include "pic.h"

static isr_t interrupt_handlers[256];

void register_interrupt_handler(uint8_t n, isr_t handler) {
    interrupt_handlers[n] = handler;
}

void isr_handler(registers_t *regs) {
    if (interrupt_handlers[regs->int_no] != NULL) {
        isr_t handler = interrupt_handlers[regs->int_no];
        handler(regs);
    }
}

void irq_handler(registers_t *regs) {
    if (regs->int_no >= 40) {
        outb(0xA0, 0x20); /* EOI to slave PIC */
    }
    outb(0x20, 0x20);     /* EOI to master PIC */

    if (interrupt_handlers[regs->int_no] != NULL) {
        isr_t handler = interrupt_handlers[regs->int_no];
        handler(regs);
    }
}

void isr_init(void) {
    memset(interrupt_handlers, 0, sizeof(interrupt_handlers));
}
