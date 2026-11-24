#include "gui.h"
#include "timer.h"
#include "net.h"
#include "apps.h"
#include "games.h"

static window_t windows[MAX_WINDOWS];
static int window_count = 0;
static int focused_win_idx = -1;

static bool start_menu_open = false;

/* Desktop Icon structure */
typedef struct {
    int x, y;
    const char *label;
    uint32_t icon_color;
    char icon_char;
    void (*on_launch)(void);
} desktop_icon_t;

static desktop_icon_t desktop_icons[] = {
    {30, 30,  "My PC",       0x00007AFF, 'C', app_sysinfo_open},
    {30, 120, "Files",       0x00FF9500, 'F', app_filemanager_open},
    {30, 210, "Terminal",    0x0034C759, '>', app_terminal_open},
    {30, 300, "Editor",      0x00AF52DE, 'E', NULL},
    {30, 390, "Calculator",  0x00FF2D55, '=', app_calculator_open},
    {30, 480, "Net Ping",    0x005856D6, '@', app_ping_open},

    {120, 30,  "Snake Game", 0x0030B0C7, 'S', game_snake_open},
    {120, 120, "Snakes&Ladd",0x00FFCC00, 'L', game_snakes_ladders_open},
    {120, 210, "Speed Car",  0x00FF3B30, 'R', game_car_racer_open}
};
static const int num_desktop_icons = sizeof(desktop_icons) / sizeof(desktop_icons[0]);

void gui_init(void) {
    memset(windows, 0, sizeof(windows));
    window_count = 0;
    focused_win_idx = -1;
    start_menu_open = false;
}

window_t *gui_create_window(const char *title, int x, int y, int w, int h, win_draw_fn on_draw) {
    if (window_count >= MAX_WINDOWS) return NULL;

    window_t *win = &windows[window_count];
    win->id = window_count;
    strncpy(win->title, title, 63);
    win->x = x;
    win->y = y;
    win->width = w;
    win->height = h;
    win->state = WIN_NORMAL;
    win->is_focused = true;
    win->is_dragging = false;
    win->bg_color = COLOR_WINDOW_BG;
    win->on_draw = on_draw;
    win->on_click = NULL;
    win->on_key = NULL;
    win->user_data = NULL;

    for (int i = 0; i < window_count; i++) {
        windows[i].is_focused = false;
    }
    focused_win_idx = window_count;
    window_count++;

    return win;
}

void gui_destroy_window(int id) {
    int found = -1;
    for (int i = 0; i < window_count; i++) {
        if (windows[i].id == id) {
            found = i;
            break;
        }
    }
    if (found == -1) return;

    for (int i = found; i < window_count - 1; i++) {
        windows[i] = windows[i + 1];
    }
    window_count--;

    if (window_count > 0) {
        focused_win_idx = window_count - 1;
        windows[focused_win_idx].is_focused = true;
    } else {
        focused_win_idx = -1;
    }
}

window_t *gui_get_focused_window(void) {
    if (focused_win_idx >= 0 && focused_win_idx < window_count) {
        return &windows[focused_win_idx];
    }
    return NULL;
}

static void draw_wallpaper(void) {
    /* Rich modern wallpaper gradient from dark indigo to deep space navy */
    gfx_draw_gradient_v(0, 0, g_fb.width, g_fb.height - TASKBAR_HEIGHT, 0x000F172A, 0x001E293B);

    /* Center watermark logo */
    int cx = g_fb.width / 2;
    int cy = (g_fb.height - TASKBAR_HEIGHT) / 2;

    gfx_draw_string(cx - 80, cy - 20, "LOHITHOS", 0x00334155, 0, true);
    gfx_draw_string(cx - 140, cy + 5, "Bare-Metal 32-bit x86 Operating System", 0x00334155, 0, true);
}

static void draw_desktop_icons(void) {
    for (int i = 0; i < num_desktop_icons; i++) {
        desktop_icon_t *icon = &desktop_icons[i];

        /* Icon tile background */
        gfx_fill_rect(icon->x, icon->y, 56, 56, icon->icon_color);
        gfx_draw_rect(icon->x, icon->y, 56, 56, COLOR_WHITE);

        /* Icon symbol */
        char sym[2] = {icon->icon_char, '\0'};
        gfx_draw_string(icon->x + 24, icon->y + 20, sym, COLOR_WHITE, 0, true);

        /* Label */
        gfx_draw_string(icon->x - 4, icon->y + 62, icon->label, COLOR_WHITE, 0, true);
    }
}

static void draw_taskbar(void) {
    int tb_y = g_fb.height - TASKBAR_HEIGHT;

    /* Taskbar background with glass finish */
    gfx_fill_rect(0, tb_y, g_fb.width, TASKBAR_HEIGHT, COLOR_TASKBAR);
    gfx_draw_line(0, tb_y, g_fb.width, tb_y, 0x003A4A5E);

    /* Start button */
    uint32_t start_col = start_menu_open ? 0x00007AFF : 0x001E2A38;
    gfx_fill_rect(6, tb_y + 5, 80, 30, start_col);
    gfx_draw_rect(6, tb_y + 5, 80, 30, COLOR_ACCENT);
    gfx_draw_string(18, tb_y + 12, "START", COLOR_WHITE, 0, true);

    /* Open window tabs on taskbar */
    int tab_x = 95;
    for (int i = 0; i < window_count; i++) {
        window_t *w = &windows[i];
        if (w->state == WIN_CLOSED) continue;

        uint32_t tab_col = (i == focused_win_idx && w->state != WIN_MINIMIZED) ? 0x002C3E50 : 0x0018222D;
        gfx_fill_rect(tab_x, tb_y + 5, 120, 30, tab_col);
        gfx_draw_rect(tab_x, tb_y + 5, 120, 30, (i == focused_win_idx) ? COLOR_ACCENT : 0x00334455);

        char title_clip[14];
        strncpy(title_clip, w->title, 13);
        title_clip[13] = '\0';
        gfx_draw_string(tab_x + 8, tb_y + 12, title_clip, COLOR_WHITE, 0, true);

        tab_x += 126;
        if (tab_x > (int)g_fb.width - 320) break;
    }

    /* System Tray (Right side) */
    int tray_x = g_fb.width - 300;

    /* Network / Wi-Fi indicator */
    gfx_fill_rect(tray_x, tb_y + 6, 175, 28, 0x0018222D);
    gfx_draw_rect(tray_x, tb_y + 6, 175, 28, 0x00334455);

    char ip_str[32];
    ip_to_str(g_net.my_ip, ip_str);
    char net_info[48];
    sprintf(net_info, "Wi-Fi: %s", ip_str);
    gfx_draw_string(tray_x + 8, tb_y + 13, net_info, COLOR_GREEN, 0, true);

    /* Live Clock */
    uint32_t sec = timer_get_uptime_sec();
    uint32_t hr = (sec / 3600) % 24;
    uint32_t min = (sec / 60) % 60;
    uint32_t s = sec % 60;

    char time_str[16];
    sprintf(time_str, "%d:%d:%d", hr, min, s);
    gfx_fill_rect(g_fb.width - 110, tb_y + 6, 100, 28, 0x0018222D);
    gfx_draw_rect(g_fb.width - 110, tb_y + 6, 100, 28, 0x00334455);
    gfx_draw_string(g_fb.width - 95, tb_y + 13, time_str, COLOR_WHITE, 0, true);
}

static void draw_start_menu(void) {
    if (!start_menu_open) return;

    int sm_w = 260;
    int sm_h = 340;
    int sm_x = 6;
    int sm_y = g_fb.height - TASKBAR_HEIGHT - sm_h - 6;

    /* Menu container */
    gfx_fill_rect(sm_x, sm_y, sm_w, sm_h, 0x001E293B);
    gfx_draw_rect(sm_x, sm_y, sm_w, sm_h, COLOR_ACCENT);

    /* Menu header */
    gfx_fill_rect(sm_x, sm_y, sm_w, 40, 0x000F172A);
    gfx_draw_string(sm_x + 16, sm_y + 14, "LohithOS Menu", COLOR_WHITE, 0, true);

    /* Menu items */
    const char *items[] = {
        "1. Diagnostic Terminal",
        "2. File Manager",
        "3. Text Editor",
        "4. Calculator",
        "5. System Information",
        "6. Network Ping Diagnostics",
        "7. Snake Food Bite Game",
        "8. Snakes & Ladders Game",
        "9. Speed Car 2D Racer"
    };

    for (int i = 0; i < 9; i++) {
        int item_y = sm_y + 48 + i * 30;
        gfx_draw_string(sm_x + 16, item_y + 7, items[i], COLOR_LIGHT_GRAY, 0, true);
        gfx_draw_line(sm_x + 10, item_y + 26, sm_x + sm_w - 10, item_y + 26, 0x00334155);
    }
}

static void draw_windows(void) {
    for (int i = 0; i < window_count; i++) {
        window_t *w = &windows[i];
        if (w->state == WIN_CLOSED || w->state == WIN_MINIMIZED) continue;

        /* Window drop shadow */
        gfx_fill_rect(w->x + 4, w->y + 4, w->width, w->height, 0x000A0E17);

        /* Window Body */
        gfx_fill_rect(w->x, w->y, w->width, w->height, w->bg_color);
        gfx_draw_rect(w->x, w->y, w->width, w->height, w->is_focused ? COLOR_ACCENT : COLOR_GRAY);

        /* Title Bar */
        uint32_t bar_col = w->is_focused ? COLOR_WINDOW_BAR : 0x002A3644;
        gfx_fill_rect(w->x, w->y, w->width, 26, bar_col);
        gfx_draw_line(w->x, w->y + 26, w->x + w->width, w->y + 26, 0x004A5A6E);

        /* Title Text */
        gfx_draw_string(w->x + 10, w->y + 5, w->title, COLOR_WHITE, 0, true);

        /* Window controls: Close [X] and Minimize [-] */
        int btn_close_x = w->x + w->width - 24;
        gfx_fill_rect(btn_close_x, w->y + 4, 18, 18, COLOR_RED);
        gfx_draw_string(btn_close_x + 5, w->y + 5, "X", COLOR_WHITE, 0, true);

        int btn_min_x = w->x + w->width - 46;
        gfx_fill_rect(btn_min_x, w->y + 4, 18, 18, COLOR_YELLOW);
        gfx_draw_string(btn_min_x + 5, w->y + 5, "-", COLOR_BLACK, 0, true);

        /* Content drawing callback */
        if (w->on_draw) {
            w->on_draw(w);
        }
    }
}

void gui_render_all(void) {
    draw_wallpaper();
    draw_desktop_icons();
    draw_windows();
    draw_taskbar();
    draw_start_menu();
    mouse_draw_cursor();
    gfx_flip();
}

void gui_handle_mouse(void) {
    int mx = g_mouse.x;
    int my = g_mouse.y;
    int tb_y = g_fb.height - TASKBAR_HEIGHT;

    /* Handle dragging active window */
    if (focused_win_idx >= 0 && windows[focused_win_idx].is_dragging) {
        if (g_mouse.left_button) {
            windows[focused_win_idx].x = mx - windows[focused_win_idx].drag_off_x;
            windows[focused_win_idx].y = my - windows[focused_win_idx].drag_off_y;
        } else {
            windows[focused_win_idx].is_dragging = false;
        }
        return;
    }

    if (g_mouse.left_clicked) {
        /* Check Start Button */
        if (mx >= 6 && mx <= 86 && my >= tb_y + 5 && my <= tb_y + 35) {
            start_menu_open = !start_menu_open;
            return;
        }

        /* Check Start Menu click */
        if (start_menu_open) {
            int sm_w = 260;
            int sm_h = 340;
            int sm_x = 6;
            int sm_y = tb_y - sm_h - 6;

            if (mx >= sm_x && mx <= sm_x + sm_w && my >= sm_y && my <= sm_y + sm_h) {
                int item_idx = (my - (sm_y + 48)) / 30;
                start_menu_open = false;
                if (item_idx == 0) app_terminal_open();
                else if (item_idx == 1) app_filemanager_open();
                else if (item_idx == 2) app_editor_open("/docs/readme.txt");
                else if (item_idx == 3) app_calculator_open();
                else if (item_idx == 4) app_sysinfo_open();
                else if (item_idx == 5) app_ping_open();
                else if (item_idx == 6) game_snake_open();
                else if (item_idx == 7) game_snakes_ladders_open();
                else if (item_idx == 8) game_car_racer_open();
                return;
            } else {
                start_menu_open = false;
            }
        }

        /* Check Taskbar tabs */
        if (my >= tb_y) {
            int tab_x = 95;
            for (int i = 0; i < window_count; i++) {
                if (windows[i].state == WIN_CLOSED) continue;
                if (mx >= tab_x && mx <= tab_x + 120) {
                    if (focused_win_idx == i && windows[i].state == WIN_NORMAL) {
                        windows[i].state = WIN_MINIMIZED;
                        focused_win_idx = -1;
                    } else {
                        windows[i].state = WIN_NORMAL;
                        for (int j = 0; j < window_count; j++) windows[j].is_focused = false;
                        windows[i].is_focused = true;
                        focused_win_idx = i;
                    }
                    return;
                }
                tab_x += 126;
            }
            return;
        }

        /* Check Windows (top to bottom Z-order) */
        for (int i = window_count - 1; i >= 0; i--) {
            window_t *w = &windows[i];
            if (w->state == WIN_CLOSED || w->state == WIN_MINIMIZED) continue;

            if (mx >= w->x && mx <= w->x + w->width && my >= w->y && my <= w->y + w->height) {
                /* Bring to top */
                for (int j = 0; j < window_count; j++) windows[j].is_focused = false;
                w->is_focused = true;
                focused_win_idx = i;

                /* Check Close Button */
                if (mx >= w->x + w->width - 24 && mx <= w->x + w->width - 6 && my >= w->y + 4 && my <= w->y + 22) {
                    gui_destroy_window(w->id);
                    return;
                }
                /* Check Minimize Button */
                if (mx >= w->x + w->width - 46 && mx <= w->x + w->width - 28 && my >= w->y + 4 && my <= w->y + 22) {
                    w->state = WIN_MINIMIZED;
                    focused_win_idx = -1;
                    return;
                }
                /* Title bar drag */
                if (my <= w->y + 26) {
                    w->is_dragging = true;
                    w->drag_off_x = mx - w->x;
                    w->drag_off_y = my - w->y;
                    return;
                }
                /* Forward click inside window content */
                if (w->on_click) {
                    w->on_click(w, mx - w->x, my - (w->y + 26), true);
                }
                return;
            }
        }

        /* Check Desktop Icons */
        for (int i = 0; i < num_desktop_icons; i++) {
            desktop_icon_t *icon = &desktop_icons[i];
            if (mx >= icon->x && mx <= icon->x + 56 && my >= icon->y && my <= icon->y + 76) {
                if (icon->on_launch) {
                    icon->on_launch();
                } else if (strcmp(icon->label, "Editor") == 0) {
                    app_editor_open("/docs/readme.txt");
                }
                return;
            }
        }
    }
}

void gui_handle_keyboard(char key) {
    if (focused_win_idx >= 0 && focused_win_idx < window_count) {
        window_t *w = &windows[focused_win_idx];
        if (w->on_key && w->state == WIN_NORMAL) {
            w->on_key(w, key);
        }
    }
}
