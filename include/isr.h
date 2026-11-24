#ifndef _ISR_H_
#define _ISR_H_

#include "types.h"

struct registers {
    uint32_t ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t int_no, err_code;
    uint32_t eip, cs, eflags, useresp, ss;
};
typedef struct registers registers_t;

typedef void (*isr_t)(registers_t *);
void register_interrupt_handler(uint8_t n, isr_t handler);
void isr_handler(registers_t *regs);
void irq_handler(registers_t *regs);
void isr_init(void);

#endif
