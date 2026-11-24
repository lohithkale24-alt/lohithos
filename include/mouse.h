#ifndef _MOUSE_H_
#define _MOUSE_H_

#include "types.h"

typedef struct {
    int x;
    int y;
    bool left_button;
    bool right_button;
    bool middle_button;
    bool left_clicked;
    bool right_clicked;
} mouse_state_t;

extern mouse_state_t g_mouse;

void mouse_init(int max_x, int max_y);
void mouse_poll(void);
void mouse_draw_cursor(void);

#endif
