#include "mouse.h"
#include "graphics.h"
#include "isr.h"
#include "pic.h"

mouse_state_t g_mouse;

static int mouse_screen_max_x = 1024;
static int mouse_screen_max_y = 768;
static uint8_t mouse_cycle = 0;
static int8_t  mouse_bytes[3];

static inline void mouse_wait(uint8_t type) {
    uint32_t timeout = 100000;
    if (type == 0) {
        while (timeout--) {
            if ((inb(0x64) & 1) == 1) return;
        }
    } else {
        while (timeout--) {
            if ((inb(0x64) & 2) == 0) return;
        }
    }
}

static inline void mouse_write(uint8_t val) {
    mouse_wait(1);
    outb(0x64, 0xD4);
    mouse_wait(1);
    outb(0x60, val);
}

static inline uint8_t mouse_read(void) {
    mouse_wait(0);
    return inb(0x60);
}

static void mouse_callback(registers_t *regs) {
    (void)regs;
    uint8_t status = inb(0x64);
    if (!(status & 0x20)) return; /* Not mouse data */

    uint8_t b = inb(0x60);

    if (mouse_cycle == 0) {
        if ((b & 0x08) == 0x08) { /* Valid sync bit */
            mouse_bytes[0] = b;
            mouse_cycle++;
        }
    } else if (mouse_cycle == 1) {
        mouse_bytes[1] = b;
        mouse_cycle++;
    } else if (mouse_cycle == 2) {
        mouse_bytes[2] = b;
        mouse_cycle = 0;

        uint8_t flags = (uint8_t)mouse_bytes[0];
        int dx = mouse_bytes[1];
        int dy = mouse_bytes[2];

        if (flags & 0x10) dx |= 0xFFFFFF00;
        if (flags & 0x20) dy |= 0xFFFFFF00;

        /* In PS/2 mouse, dy is inverted */
        g_mouse.x += dx;
        g_mouse.y -= dy;

        /* Clamp to screen dimensions */
        if (g_mouse.x < 0) g_mouse.x = 0;
        if (g_mouse.y < 0) g_mouse.y = 0;
        if (g_mouse.x >= mouse_screen_max_x) g_mouse.x = mouse_screen_max_x - 1;
        if (g_mouse.y >= mouse_screen_max_y) g_mouse.y = mouse_screen_max_y - 1;

        bool prev_left = g_mouse.left_button;
        bool prev_right = g_mouse.right_button;

        g_mouse.left_button   = (flags & 0x01) != 0;
        g_mouse.right_button  = (flags & 0x02) != 0;
        g_mouse.middle_button = (flags & 0x04) != 0;

        g_mouse.left_clicked  = (!prev_left && g_mouse.left_button);
        g_mouse.right_clicked = (!prev_right && g_mouse.right_button);
    }
}

void mouse_init(int max_x, int max_y) {
    mouse_screen_max_x = max_x;
    mouse_screen_max_y = max_y;
    g_mouse.x = max_x / 2;
    g_mouse.y = max_y / 2;
    g_mouse.left_button = false;
    g_mouse.right_button = false;
    g_mouse.middle_button = false;
    g_mouse.left_clicked = false;
    g_mouse.right_clicked = false;

    /* Enable PS/2 Auxiliary Device */
    mouse_wait(1);
    outb(0x64, 0xA8);

    /* Read Compaq status byte */
    mouse_wait(1);
    outb(0x64, 0x20);
    uint8_t status = mouse_read();
    status |= 0x02;   /* Enable IRQ12 */
    status &= ~0x20;  /* Enable mouse clock */

    /* Write back Compaq status byte */
    mouse_wait(1);
    outb(0x64, 0x60);
    mouse_wait(1);
    outb(0x60, status);

    /* Set default settings */
    mouse_write(0xF6);
    mouse_read(); /* ACK */

    /* Enable data streaming */
    mouse_write(0xF4);
    mouse_read(); /* ACK */

    register_interrupt_handler(44, mouse_callback); /* IRQ 12 -> INT 44 */
    pic_clear_mask(12); /* Unmask IRQ12 */
    pic_clear_mask(2);  /* Unmask IRQ2 (cascade) */
}

/* Elegant Anti-Aliased Arrow Mouse Cursor Bitmap */
static const char *cursor_shape[] = {
    "X               ",
    "XX              ",
    "X.X             ",
    "X..X            ",
    "X...X           ",
    "X....X          ",
    "X.....X         ",
    "X......X        ",
    "X.......X       ",
    "X........X      ",
    "X.....XXXXX     ",
    "X..X..X         ",
    "X.X X..X        ",
    "XX   X..X       ",
    "X     X..X      ",
    "       XX       "
};

void mouse_draw_cursor(void) {
    int mx = g_mouse.x;
    int my = g_mouse.y;

    for (int r = 0; r < 16; r++) {
        for (int c = 0; c < 16; c++) {
            char ch = cursor_shape[r][c];
            if (ch == 'X') {
                gfx_putpixel(mx + c, my + r, COLOR_BLACK);
            } else if (ch == '.') {
                gfx_putpixel(mx + c, my + r, COLOR_WHITE);
            }
        }
    }
}
