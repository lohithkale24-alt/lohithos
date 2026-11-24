#ifndef _GRAPHICS_H_
#define _GRAPHICS_H_

#include "types.h"
#include "multiboot.h"

#define COLOR_BLACK       0x00000000
#define COLOR_WHITE       0x00FFFFFF
#define COLOR_RED         0x00FF3B30
#define COLOR_GREEN       0x0034C759
#define COLOR_BLUE        0x00007AFF
#define COLOR_YELLOW      0x00FFCC00
#define COLOR_ORANGE      0x00FF9500
#define COLOR_PURPLE      0x00AF52DE
#define COLOR_GRAY        0x008E8E93
#define COLOR_LIGHT_GRAY  0x00D1D1D6
#define COLOR_DARK_GRAY   0x001C1C1E
#define COLOR_NAVY        0x000A192F
#define COLOR_CYAN        0x0032ADE6
#define COLOR_TEAL        0x0030B0C7
#define COLOR_WINDOW_BG   0x00F2F2F7
#define COLOR_WINDOW_BAR  0x001A2634
#define COLOR_TASKBAR     0x00101820
#define COLOR_ACCENT      0x000080FF

typedef struct {
    uint32_t *vram;
    uint32_t *backbuffer;
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint8_t  bpp;
} framebuffer_t;

extern framebuffer_t g_fb;

void gfx_init(multiboot_info_t *mbi);
void gfx_flip(void);
void gfx_clear(uint32_t color);
void gfx_putpixel(int x, int y, uint32_t color);
uint32_t gfx_getpixel(int x, int y);
void gfx_draw_line(int x0, int y0, int x1, int y1, uint32_t color);
void gfx_draw_rect(int x, int y, int w, int h, uint32_t color);
void gfx_fill_rect(int x, int y, int w, int h, uint32_t color);
void gfx_draw_rounded_rect(int x, int y, int w, int h, int r, uint32_t color);
void gfx_fill_rounded_rect(int x, int y, int w, int h, int r, uint32_t color);
void gfx_draw_circle(int xc, int yc, int r, uint32_t color);
void gfx_fill_circle(int xc, int yc, int r, uint32_t color);
void gfx_draw_gradient_v(int x, int y, int w, int h, uint32_t top_color, uint32_t bot_color);
void gfx_draw_char(int x, int y, char c, uint32_t color, uint32_t bg_color, bool transparent);
void gfx_draw_string(int x, int y, const char *str, uint32_t color, uint32_t bg_color, bool transparent);

#endif
