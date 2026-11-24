#ifndef _GUI_H_
#define _GUI_H_

#include "types.h"
#include "graphics.h"
#include "mouse.h"

#define MAX_WINDOWS 16
#define TASKBAR_HEIGHT 40

typedef enum {
    WIN_NORMAL,
    WIN_MINIMIZED,
    WIN_MAXIMIZED,
    WIN_CLOSED
} window_state_t;

typedef struct window window_t;

typedef void (*win_draw_fn)(window_t *win);
typedef void (*win_click_fn)(window_t *win, int rel_x, int rel_y, bool left_click);
typedef void (*win_key_fn)(window_t *win, char key);

struct window {
    int id;
    char title[64];
    int x, y, width, height;
    window_state_t state;
    bool is_focused;
    bool is_dragging;
    int drag_off_x, drag_off_y;
    uint32_t bg_color;
    win_draw_fn on_draw;
    win_click_fn on_click;
    win_key_fn on_key;
    void *user_data;
};

void gui_init(void);
window_t *gui_create_window(const char *title, int x, int y, int w, int h, win_draw_fn on_draw);
void gui_destroy_window(int id);
void gui_render_all(void);
void gui_handle_mouse(void);
void gui_handle_keyboard(char key);
window_t *gui_get_focused_window(void);

#endif
