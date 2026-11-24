#include "graphics.h"
#include "font.h"
#include "heap.h"

framebuffer_t g_fb;
static uint32_t s_backbuffer_storage[1024 * 768]; /* Fixed 3MB backbuffer in BSS */

void gfx_init(multiboot_info_t *mbi) {
    if (mbi && (mbi->flags & MULTIBOOT_INFO_FRAMEBUFFER) && mbi->framebuffer_addr != 0) {
        g_fb.vram   = (uint32_t *)((uint32_t)mbi->framebuffer_addr);
        g_fb.width  = mbi->framebuffer_width;
        g_fb.height = mbi->framebuffer_height;
        g_fb.pitch  = mbi->framebuffer_pitch;
        g_fb.bpp    = mbi->framebuffer_bpp;
    } else {
        /* VirtualBox Default VBE Linear Framebuffer fallback */
        g_fb.vram   = (uint32_t *)0xE0000000;
        g_fb.width  = 1024;
        g_fb.height = 768;
        g_fb.pitch  = 1024 * 4;
        g_fb.bpp    = 32;
    }

    g_fb.backbuffer = s_backbuffer_storage;
    gfx_clear(COLOR_NAVY);
    gfx_flip();
}

void gfx_flip(void) {
    uint32_t total_pixels = g_fb.width * g_fb.height;
    uint32_t *src = g_fb.backbuffer;
    uint32_t *dst = g_fb.vram;

    /* Fast 32-bit copy */
    for (uint32_t i = 0; i < total_pixels; i++) {
        dst[i] = src[i];
    }
}

void gfx_clear(uint32_t color) {
    uint32_t total = g_fb.width * g_fb.height;
    uint32_t *buf = g_fb.backbuffer;
    for (uint32_t i = 0; i < total; i++) {
        buf[i] = color;
    }
}

void gfx_putpixel(int x, int y, uint32_t color) {
    if (x < 0 || x >= (int)g_fb.width || y < 0 || y >= (int)g_fb.height) {
        return;
    }
    g_fb.backbuffer[y * g_fb.width + x] = color;
}

uint32_t gfx_getpixel(int x, int y) {
    if (x < 0 || x >= (int)g_fb.width || y < 0 || y >= (int)g_fb.height) {
        return 0;
    }
    return g_fb.backbuffer[y * g_fb.width + x];
}

void gfx_draw_line(int x0, int y0, int x1, int y1, uint32_t color) {
    int dx = (x1 > x0) ? (x1 - x0) : (x0 - x1);
    int dy = (y1 > y0) ? (y1 - y0) : (y0 - y1);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;

    while (1) {
        gfx_putpixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
}

void gfx_draw_rect(int x, int y, int w, int h, uint32_t color) {
    for (int i = 0; i < w; i++) {
        gfx_putpixel(x + i, y, color);
        gfx_putpixel(x + i, y + h - 1, color);
    }
    for (int j = 0; j < h; j++) {
        gfx_putpixel(x, y + j, color);
        gfx_putpixel(x + w - 1, y + j, color);
    }
}

void gfx_fill_rect(int x, int y, int w, int h, uint32_t color) {
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > (int)g_fb.width)  w = (int)g_fb.width - x;
    if (y + h > (int)g_fb.height) h = (int)g_fb.height - y;
    if (w <= 0 || h <= 0) return;

    for (int j = 0; j < h; j++) {
        uint32_t *row = &g_fb.backbuffer[(y + j) * g_fb.width + x];
        for (int i = 0; i < w; i++) {
            row[i] = color;
        }
    }
}

void gfx_draw_rounded_rect(int x, int y, int w, int h, int r, uint32_t color) {
    (void)r;
    gfx_draw_rect(x, y, w, h, color);
}

void gfx_fill_rounded_rect(int x, int y, int w, int h, int r, uint32_t color) {
    (void)r;
    gfx_fill_rect(x, y, w, h, color);
}

void gfx_draw_circle(int xc, int yc, int r, uint32_t color) {
    int x = 0;
    int y = r;
    int d = 3 - 2 * r;
    while (y >= x) {
        gfx_putpixel(xc + x, yc + y, color);
        gfx_putpixel(xc - x, yc + y, color);
        gfx_putpixel(xc + x, yc - y, color);
        gfx_putpixel(xc - x, yc - y, color);
        gfx_putpixel(xc + y, yc + x, color);
        gfx_putpixel(xc - y, yc + x, color);
        gfx_putpixel(xc + y, yc - x, color);
        gfx_putpixel(xc - y, yc - x, color);
        x++;
        if (d > 0) {
            y--;
            d = d + 4 * (x - y) + 10;
        } else {
            d = d + 4 * x + 6;
        }
    }
}

void gfx_fill_circle(int xc, int yc, int r, uint32_t color) {
    for (int y = -r; y <= r; y++) {
        for (int x = -r; x <= r; x++) {
            if (x * x + y * y <= r * r) {
                gfx_putpixel(xc + x, yc + y, color);
            }
        }
    }
}

void gfx_draw_gradient_v(int x, int y, int w, int h, uint32_t top_color, uint32_t bot_color) {
    uint8_t r1 = (top_color >> 16) & 0xFF, g1 = (top_color >> 8) & 0xFF, b1 = top_color & 0xFF;
    uint8_t r2 = (bot_color >> 16) & 0xFF, g2 = (bot_color >> 8) & 0xFF, b2 = bot_color & 0xFF;

    for (int j = 0; j < h; j++) {
        uint8_t r = (uint8_t)(r1 + ((r2 - r1) * j) / h);
        uint8_t g = (uint8_t)(g1 + ((g2 - g1) * j) / h);
        uint8_t b = (uint8_t)(b1 + ((b2 - b1) * j) / h);
        uint32_t col = (r << 16) | (g << 8) | b;

        uint32_t *row = &g_fb.backbuffer[(y + j) * g_fb.width + x];
        for (int i = 0; i < w; i++) {
            row[i] = col;
        }
    }
}

void gfx_draw_char(int x, int y, char c, uint32_t color, uint32_t bg_color, bool transparent) {
    uint8_t uc = (uint8_t)c;
    const uint8_t *glyph = font_bitmap[uc];

    for (int row = 0; row < 16; row++) {
        uint8_t bits = glyph[row];
        for (int col = 0; col < 8; col++) {
            if (bits & (0x80 >> col)) {
                gfx_putpixel(x + col, y + row, color);
            } else if (!transparent) {
                gfx_putpixel(x + col, y + row, bg_color);
            }
        }
    }
}

void gfx_draw_string(int x, int y, const char *str, uint32_t color, uint32_t bg_color, bool transparent) {
    if (!str) return;
    int cur_x = x;
    int cur_y = y;

    while (*str) {
        if (*str == '\n') {
            cur_x = x;
            cur_y += 18;
        } else if (*str == '\t') {
            cur_x += 8 * 4;
        } else {
            gfx_draw_char(cur_x, cur_y, *str, color, bg_color, transparent);
            cur_x += 8;
        }
        str++;
    }
}
