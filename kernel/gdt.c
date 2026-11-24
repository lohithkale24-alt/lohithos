#include "gdt.h"

struct gdt_entry g_gdt[5];
struct gdt_ptr   g_gdt_ptr;

extern void gdt_flush(void);

static void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    g_gdt[num].base_low    = (base & 0xFFFF);
    g_gdt[num].base_middle = (base >> 16) & 0xFF;
    g_gdt[num].base_high   = (base >> 24) & 0xFF;

    g_gdt[num].limit_low   = (limit & 0xFFFF);
    g_gdt[num].granularity = ((limit >> 16) & 0x0F) | (gran & 0xF0);
    g_gdt[num].access      = access;
}

void gdt_init(void) {
    g_gdt_ptr.limit = (sizeof(struct gdt_entry) * 5) - 1;
    g_gdt_ptr.base  = (uint32_t)&g_gdt;

    /* 0: Null descriptor */
    gdt_set_gate(0, 0, 0, 0, 0);

    /* 1: Kernel Code Segment (0x08), Base 0, Limit 4GB, 32-bit, Ring 0 */
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);

    /* 2: Kernel Data Segment (0x10), Base 0, Limit 4GB, 32-bit, Ring 0 */
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF);

    /* 3: User Code Segment (0x18), Base 0, Limit 4GB, 32-bit, Ring 3 */
    gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF);

    /* 4: User Data Segment (0x20), Base 0, Limit 4GB, 32-bit, Ring 3 */
    gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF);

    gdt_flush();
}
