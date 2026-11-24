#include "timer.h"
#include "isr.h"

static volatile uint32_t timer_ticks = 0;

static void timer_callback(registers_t *regs) {
    (void)regs;
    timer_ticks++;
}

void timer_init(uint32_t frequency) {
    register_interrupt_handler(32, timer_callback);

    uint32_t divisor = 1193180 / frequency;
    outb(0x43, 0x36); /* Command byte: Channel 0, lobyte/hibyte, Mode 3 (Square Wave) */

    uint8_t l = (uint8_t)(divisor & 0xFF);
    uint8_t h = (uint8_t)((divisor >> 8) & 0xFF);

    outb(0x40, l);
    outb(0x40, h);
}

uint32_t timer_get_ticks(void) {
    return timer_ticks;
}

uint32_t timer_get_uptime_sec(void) {
    return timer_ticks / 100; /* Frequency is 100 Hz */
}

void timer_sleep_ms(uint32_t ms) {
    uint32_t target = timer_ticks + (ms / 10);
    while (timer_ticks < target) {
        hlt();
    }
}
