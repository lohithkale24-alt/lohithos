#include "games.h"
#include "graphics.h"
#include "keyboard.h"
#include "timer.h"

/* ========================================================================= */
/* 1. SNAKE FOOD BITE GAME                                                   */
/* ========================================================================= */
#define SNAKE_GRID_W 24
#define SNAKE_GRID_H 18
#define SNAKE_CELL_SIZE 16

typedef struct {
    int x, y;
} point_t;

static point_t snake_body[SNAKE_GRID_W * SNAKE_GRID_H];
static int     snake_len = 4;
static int     snake_dir = 1; /* 0: Up, 1: Right, 2: Down, 3: Left */
static point_t snake_food = {12, 9};
static int     snake_score = 0;
static bool    snake_game_over = false;
static uint32_t snake_last_tick = 0;

static void snake_reset(void) {
    snake_len = 4;
    snake_dir = 1;
    snake_score = 0;
    snake_game_over = false;
    snake_body[0].x = 8; snake_body[0].y = 8;
    snake_body[1].x = 7; snake_body[1].y = 8;
    snake_body[2].x = 6; snake_body[2].y = 8;
    snake_body[3].x = 5; snake_body[3].y = 8;
    snake_food.x = 16;
    snake_food.y = 8;
}

static void snake_update(void) {
    if (snake_game_over) return;

    point_t new_head = snake_body[0];
    if (snake_dir == 0) new_head.y--;
    else if (snake_dir == 1) new_head.x++;
    else if (snake_dir == 2) new_head.y++;
    else if (snake_dir == 3) new_head.x--;

    /* Wall collision */
    if (new_head.x < 0 || new_head.x >= SNAKE_GRID_W || new_head.y < 0 || new_head.y >= SNAKE_GRID_H) {
        snake_game_over = true;
        return;
    }

    /* Self collision */
    for (int i = 0; i < snake_len; i++) {
        if (snake_body[i].x == new_head.x && snake_body[i].y == new_head.y) {
            snake_game_over = true;
            return;
        }
    }

    /* Shift body */
    for (int i = snake_len; i > 0; i--) {
        snake_body[i] = snake_body[i - 1];
    }
    snake_body[0] = new_head;

    /* Eat food */
    if (new_head.x == snake_food.x && new_head.y == snake_food.y) {
        snake_len++;
        snake_score += 10;
        /* Spawn new food */
        snake_food.x = (snake_food.x * 7 + 3) % SNAKE_GRID_W;
        snake_food.y = (snake_food.y * 5 + 2) % SNAKE_GRID_H;
    }
}

static void snake_draw(window_t *win) {
    uint32_t now = timer_get_ticks();
    if (now - snake_last_tick >= 12) { /* ~120ms tick */
        snake_update();
        snake_last_tick = now;
    }

    int ox = win->x + 10;
    int oy = win->y + 32;

    /* Grid background */
    gfx_fill_rect(ox, oy, SNAKE_GRID_W * SNAKE_CELL_SIZE, SNAKE_GRID_H * SNAKE_CELL_SIZE, 0x000F172A);
    gfx_draw_rect(ox, oy, SNAKE_GRID_W * SNAKE_CELL_SIZE, SNAKE_GRID_H * SNAKE_CELL_SIZE, COLOR_ACCENT);

    /* Draw food (red apple) */
    int fx = ox + snake_food.x * SNAKE_CELL_SIZE;
    int fy = oy + snake_food.y * SNAKE_CELL_SIZE;
    gfx_fill_circle(fx + SNAKE_CELL_SIZE / 2, fy + SNAKE_CELL_SIZE / 2, SNAKE_CELL_SIZE / 2 - 2, COLOR_RED);

    /* Draw snake segments */
    for (int i = 0; i < snake_len; i++) {
        int sx = ox + snake_body[i].x * SNAKE_CELL_SIZE;
        int sy = oy + snake_body[i].y * SNAKE_CELL_SIZE;
        uint32_t col = (i == 0) ? 0x0010B981 : 0x00059669;
        gfx_fill_rect(sx + 1, sy + 1, SNAKE_CELL_SIZE - 2, SNAKE_CELL_SIZE - 2, col);
    }

    /* Score banner */
    char buf[48];
    sprintf(buf, "Score: %d | Arrows to steer | Space: Restart", snake_score);
    gfx_draw_string(ox, oy + SNAKE_GRID_H * SNAKE_CELL_SIZE + 10, buf, 0x000F172A, 0, true);

    if (snake_game_over) {
        int cx = ox + (SNAKE_GRID_W * SNAKE_CELL_SIZE) / 2 - 60;
        int cy = oy + (SNAKE_GRID_H * SNAKE_CELL_SIZE) / 2 - 16;
        gfx_fill_rect(cx - 10, cy - 8, 140, 36, 0x00DC2626);
        gfx_draw_string(cx, cy, "GAME OVER!", COLOR_WHITE, 0, true);
    }
}

static void snake_key(window_t *win, char key) {
    (void)win;
    if (key == (char)KEY_UP && snake_dir != 2) snake_dir = 0;
    else if (key == (char)KEY_RIGHT && snake_dir != 3) snake_dir = 1;
    else if (key == (char)KEY_DOWN && snake_dir != 0) snake_dir = 2;
    else if (key == (char)KEY_LEFT && snake_dir != 1) snake_dir = 3;
    else if (key == ' ' || key == 'r') {
        snake_reset();
    }
}

void game_snake_open(void) {
    snake_reset();
    window_t *win = gui_create_window("Snake Food Bite", 260, 90, 405, 360, snake_draw);
    if (win) {
        win->on_key = snake_key;
    }
}

/* ========================================================================= */
/* 2. SNAKES AND LADDERS GAME                                               */
/* ========================================================================= */
static int snl_player_pos = 1;
static int snl_last_dice = 1;
static char snl_msg[64] = "Click 'ROLL DICE' to start!";

static int snl_check_snakes_ladders(int pos) {
    /* Ladders */
    if (pos == 4)  return 14;
    if (pos == 9)  return 22;
    if (pos == 18) return 29;
    /* Snakes */
    if (pos == 17) return 7;
    if (pos == 25) return 10;
    if (pos == 32) return 15;
    return pos;
}

static void snl_roll(void) {
    if (snl_player_pos >= 36) {
        snl_player_pos = 1;
    }
    snl_last_dice = (timer_get_ticks() % 6) + 1;
    snl_player_pos += snl_last_dice;

    if (snl_player_pos > 36) {
        snl_player_pos = 36;
    }

    int after = snl_check_snakes_ladders(snl_player_pos);
    if (after > snl_player_pos) {
        sprintf(snl_msg, "Rolled %d! CLIMBED A LADDER to %d!", snl_last_dice, after);
        snl_player_pos = after;
    } else if (after < snl_player_pos) {
        sprintf(snl_msg, "Rolled %d! BITTEN BY A SNAKE to %d!", snl_last_dice, after);
        snl_player_pos = after;
    } else {
        if (snl_player_pos == 36) {
            sprintf(snl_msg, "Rolled %d! YOU WON! Reached 36!", snl_last_dice);
        } else {
            sprintf(snl_msg, "Rolled %d! Moved to tile %d.", snl_last_dice, snl_player_pos);
        }
    }
}

static void snl_click(window_t *win, int rel_x, int rel_y, bool left_click) {
    (void)win; (void)left_click;
    /* Roll Button is at bottom right */
    if (rel_x >= win->width - 130 && rel_x <= win->width - 15 && rel_y >= win->height - 50 && rel_y <= win->height - 18) {
        snl_roll();
    }
}

static void snl_draw(window_t *win) {
    /* Board: 6x6 grid of tiles numbered 1 to 36 */
    int ox = win->x + 15;
    int oy = win->y + 35;
    int tile_sz = 44;

    for (int r = 0; r < 6; r++) {
        for (int c = 0; c < 6; c++) {
            /* Zig-zag board numbering */
            int row_from_bottom = 5 - r;
            int num;
            if (row_from_bottom % 2 == 0) {
                num = row_from_bottom * 6 + c + 1;
            } else {
                num = row_from_bottom * 6 + (5 - c) + 1;
            }

            int tx = ox + c * tile_sz;
            int ty = oy + r * tile_sz;

            uint32_t bg = ((r + c) % 2 == 0) ? 0x00F8FAFC : 0x00E2E8F0;
            if (num == 4 || num == 9 || num == 18) bg = 0x00DCFCE7; /* Ladder start (green) */
            if (num == 17 || num == 25 || num == 32) bg = 0x00FEE2E2; /* Snake head (red) */

            gfx_fill_rect(tx, ty, tile_sz, tile_sz, bg);
            gfx_draw_rect(tx, ty, tile_sz, tile_sz, 0x0094A3B8);

            char nstr[8];
            sprintf(nstr, "%d", num);
            gfx_draw_string(tx + 4, ty + 4, nstr, 0x00475569, 0, true);

            /* Draw Player token */
            if (num == snl_player_pos) {
                gfx_fill_circle(tx + tile_sz / 2, ty + tile_sz / 2, 12, 0x002563EB);
                gfx_draw_string(tx + tile_sz / 2 - 4, ty + tile_sz / 2 - 7, "P", COLOR_WHITE, 0, true);
            }
        }
    }

    /* Right side controls */
    int ctrl_x = ox + 6 * tile_sz + 15;
    gfx_draw_string(ctrl_x, oy + 10, "SNAKES & LADDERS", 0x000F172A, 0, true);
    gfx_draw_string(ctrl_x, oy + 32, "Ladders: 4->14, 9->22", 0x0016A34A, 0, true);
    gfx_draw_string(ctrl_x, oy + 50, "Snakes:  17->7, 25->10", 0x00DC2626, 0, true);

    /* Dice Box */
    gfx_fill_rect(ctrl_x + 30, oy + 90, 60, 60, COLOR_WHITE);
    gfx_draw_rect(ctrl_x + 30, oy + 90, 60, 60, 0x000F172A);

    char dice_str[8];
    sprintf(dice_str, "%d", snl_last_dice);
    gfx_draw_string(ctrl_x + 55, oy + 112, dice_str, 0x002563EB, 0, true);

    /* Roll Button */
    int btn_x = win->x + win->width - 130;
    int btn_y = win->y + win->height - 45;
    gfx_fill_rect(btn_x, btn_y, 115, 32, 0x002563EB);
    gfx_draw_string(btn_x + 16, btn_y + 8, "ROLL DICE", COLOR_WHITE, 0, true);

    /* Message bar */
    gfx_fill_rect(ox, oy + 6 * tile_sz + 10, win->width - 30, 26, 0x000F172A);
    gfx_draw_string(ox + 8, oy + 6 * tile_sz + 15, snl_msg, 0x0038BDF8, 0, true);
}

void game_snakes_ladders_open(void) {
    snl_player_pos = 1;
    snl_last_dice = 1;
    strcpy(snl_msg, "Roll dice to reach tile 36!");
    window_t *win = gui_create_window("Snakes & Ladders Board", 200, 100, 480, 365, snl_draw);
    if (win) {
        win->on_click = snl_click;
    }
}

/* ========================================================================= */
/* 3. SPEED CAR 2D RETRO HIGHWAY RACER                                       */
/* ========================================================================= */
#define ROAD_W 180
#define ROAD_H 250

static int  car_x = 80; /* Relative to road */
static int  road_scroll = 0;
static int  obs_x = 40;
static int  obs_y = -40;
static int  car_score = 0;
static bool car_crashed = false;
static uint32_t car_last_tick = 0;

static void car_reset(void) {
    car_x = 80;
    obs_x = 40;
    obs_y = -40;
    car_score = 0;
    car_crashed = false;
}

static void car_update(void) {
    if (car_crashed) return;

    road_scroll = (road_scroll + 6) % 30;
    obs_y += 7;

    if (obs_y > ROAD_H) {
        obs_y = -40;
        obs_x = (obs_x * 13 + 37) % (ROAD_W - 40);
        car_score += 10;
    }

    /* Collision detection with car bounding box */
    if (obs_y + 36 >= ROAD_H - 50 && obs_y <= ROAD_H - 10) {
        if (car_x < obs_x + 28 && car_x + 28 > obs_x) {
            car_crashed = true;
        }
    }
}

static void car_draw(window_t *win) {
    uint32_t now = timer_get_ticks();
    if (now - car_last_tick >= 5) { /* 50ms tick */
        car_update();
        car_last_tick = now;
    }

    int rx = win->x + 20;
    int ry = win->y + 35;

    /* Asphalt Road */
    gfx_fill_rect(rx, ry, ROAD_W, ROAD_H, 0x00334155);
    gfx_draw_rect(rx, ry, ROAD_W, ROAD_H, COLOR_WHITE);

    /* Road center dashed line */
    int mid = rx + ROAD_W / 2;
    for (int y = -30 + road_scroll; y < ROAD_H; y += 30) {
        if (y >= 0 && y + 16 <= ROAD_H) {
            gfx_fill_rect(mid - 2, ry + y, 4, 16, COLOR_YELLOW);
        }
    }

    /* Player Car (Red Sports Car) */
    int px = rx + car_x;
    int py = ry + ROAD_H - 46;
    gfx_fill_rect(px + 4, py, 20, 36, COLOR_RED);
    gfx_fill_rect(px + 2, py + 6, 4, 8, COLOR_BLACK);   /* Left front wheel */
    gfx_fill_rect(px + 22, py + 6, 4, 8, COLOR_BLACK);  /* Right front wheel */
    gfx_fill_rect(px + 2, py + 22, 4, 8, COLOR_BLACK);  /* Left rear wheel */
    gfx_fill_rect(px + 22, py + 22, 4, 8, COLOR_BLACK); /* Right rear wheel */
    gfx_fill_rect(px + 6, py + 8, 16, 10, 0x0038BDF8);  /* Windshield */

    /* Obstacle Car (Blue Car) */
    if (obs_y >= -36 && obs_y <= ROAD_H) {
        int ox = rx + obs_x;
        int oy = ry + obs_y;
        gfx_fill_rect(ox + 4, oy, 20, 36, 0x002563EB);
        gfx_fill_rect(ox + 2, oy + 6, 4, 8, COLOR_BLACK);
        gfx_fill_rect(ox + 22, oy + 6, 4, 8, COLOR_BLACK);
        gfx_fill_rect(ox + 2, oy + 22, 4, 8, COLOR_BLACK);
        gfx_fill_rect(ox + 22, oy + 22, 4, 8, COLOR_BLACK);
        gfx_fill_rect(ox + 6, oy + 16, 16, 10, 0x0094A3B8);
    }

    /* Dashboard Info */
    int dx = rx + ROAD_W + 16;
    gfx_draw_string(dx, ry + 10, "SPEED CAR RACER", 0x000F172A, 0, true);

    char sbuf[32];
    sprintf(sbuf, "Score: %d pts", car_score);
    gfx_draw_string(dx, ry + 40, sbuf, 0x0016A34A, 0, true);
    gfx_draw_string(dx, ry + 70, "Left / Right Arrows", 0x00475569, 0, true);
    gfx_draw_string(dx, ry + 90, "to steer car.", 0x00475569, 0, true);
    gfx_draw_string(dx, ry + 120, "Dodge traffic!", 0x000F172A, 0, true);

    if (car_crashed) {
        gfx_fill_rect(dx, ry + 160, 130, 32, COLOR_RED);
        gfx_draw_string(dx + 12, ry + 168, "CRASHED!", COLOR_WHITE, 0, true);
        gfx_draw_string(dx, ry + 200, "Press Space to Restart", 0x000F172A, 0, true);
    }
}

static void car_key(window_t *win, char key) {
    (void)win;
    if (key == (char)KEY_LEFT || key == 'a' || key == 'A') {
        if (car_x > 6) car_x -= 14;
    } else if (key == (char)KEY_RIGHT || key == 'd' || key == 'D') {
        if (car_x < ROAD_W - 34) car_x += 14;
    } else if (key == ' ' || key == 'r' || key == 'R') {
        car_reset();
    }
}

void game_car_racer_open(void) {
    car_reset();
    window_t *win = gui_create_window("Speed Car 2D Racer", 280, 110, 390, 310, car_draw);
    if (win) {
        win->on_key = car_key;
    }
}
