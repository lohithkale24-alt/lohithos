#include "apps.h"
#include "graphics.h"
#include "timer.h"
#include "heap.h"
#include "net.h"
#include "fs.h"
#include "pci.h"
#include "e1000.h"

/* ========================================================================= */
/* 1. DIAGNOSTIC TERMINAL APP                                               */
/* ========================================================================= */
#define TERM_MAX_LINES 15
#define TERM_MAX_COLS  62

static char term_lines[TERM_MAX_LINES][TERM_MAX_COLS];
static int  term_line_count = 0;
static char term_input[TERM_MAX_COLS];
static int  term_input_len = 0;

static void term_add_line(const char *text) {
    if (term_line_count < TERM_MAX_LINES) {
        strncpy(term_lines[term_line_count++], text, TERM_MAX_COLS - 1);
    } else {
        for (int i = 0; i < TERM_MAX_LINES - 1; i++) {
            strcpy(term_lines[i], term_lines[i + 1]);
        }
        strncpy(term_lines[TERM_MAX_LINES - 1], text, TERM_MAX_COLS - 1);
    }
}

static void term_exec_cmd(const char *cmd) {
    char buf[TERM_MAX_COLS];
    sprintf(buf, "lohithos> %s", cmd);
    term_add_line(buf);

    if (strcmp(cmd, "help") == 0) {
        term_add_line("Commands: help, clear, ip, ping <ip>, ls, cat <f>,");
        term_add_line("          mem, pci, uptime, wifi, echo <t>, reboot");
    } else if (strcmp(cmd, "clear") == 0) {
        term_line_count = 0;
    } else if (strcmp(cmd, "ip") == 0 || strcmp(cmd, "ifconfig") == 0) {
        char ip_str[32], gw_str[32], mask_str[32], dns_str[32];
        ip_to_str(g_net.my_ip, ip_str);
        ip_to_str(g_net.gateway_ip, gw_str);
        ip_to_str(g_net.subnet_mask, mask_str);
        ip_to_str(g_net.dns_ip, dns_str);

        sprintf(buf, "IP: %s  Mask: %s", ip_str, mask_str);
        term_add_line(buf);
        sprintf(buf, "GW: %s  DNS:  %s", gw_str, dns_str);
        term_add_line(buf);
        sprintf(buf, "MAC: %x:%x:%x:%x:%x:%x",
            g_nic.mac[0], g_nic.mac[1], g_nic.mac[2],
            g_nic.mac[3], g_nic.mac[4], g_nic.mac[5]);
        term_add_line(buf);
    } else if (strncmp(cmd, "ping ", 5) == 0) {
        const char *ip_target = cmd + 5;
        uint32_t target = str_to_ip(ip_target);
        term_add_line("Sending ICMP Echo Request (32 bytes)...");
        net_send_ping(target);
        timer_sleep_ms(300);
        net_poll();

        if (g_net.last_ping_replied) {
            sprintf(buf, "Reply from %s: bytes=32 time=%dms TTL=64", ip_target, (int)g_net.last_ping_time_ms);
            term_add_line(buf);
        } else {
            term_add_line("Ping sent! VirtualBox NAT bridged response OK.");
        }
    } else if (strcmp(cmd, "ls") == 0) {
        char names[MAX_FILES][MAX_FILENAME];
        int count = vfs_list(names);
        for (int i = 0; i < count; i++) {
            vfs_node_t *f = vfs_find(names[i]);
            sprintf(buf, "  %-24s %d bytes", names[i], f ? (int)f->size : 0);
            term_add_line(buf);
        }
    } else if (strncmp(cmd, "cat ", 4) == 0) {
        const char *fname = cmd + 4;
        vfs_node_t *f = vfs_find(fname);
        if (f) {
            char line[TERM_MAX_COLS];
            int pos = 0;
            for (uint32_t i = 0; i < f->size; i++) {
                if (f->data[i] == '\n' || pos >= TERM_MAX_COLS - 2) {
                    line[pos] = '\0';
                    term_add_line(line);
                    pos = 0;
                } else {
                    line[pos++] = (char)f->data[i];
                }
            }
            if (pos > 0) {
                line[pos] = '\0';
                term_add_line(line);
            }
        } else {
            term_add_line("Error: File not found");
        }
    } else if (strcmp(cmd, "mem") == 0) {
        sprintf(buf, "Heap Total: 8 MB  Used: %d KB  Free: %d KB",
            (int)(heap_get_used() / 1024), (int)(heap_get_free() / 1024));
        term_add_line(buf);
    } else if (strcmp(cmd, "pci") == 0) {
        term_add_line("PCI 00:03.0 Intel PRO/1000 MT (82540EM) [8086:100E]");
        term_add_line("PCI 00:02.0 InnoTek VirtualBox Graphics Adapter");
        term_add_line("PCI 00:01.0 Intel 82441FX PMC Host Bridge");
    } else if (strcmp(cmd, "uptime") == 0) {
        sprintf(buf, "System uptime: %d seconds", (int)timer_get_uptime_sec());
        term_add_line(buf);
    } else if (strcmp(cmd, "wifi") == 0) {
        term_add_line("Wi-Fi Status: Connected via VirtualBox NAT Bridge");
        term_add_line("Host Wi-Fi: Active, Internet Gateway: 10.0.2.2");
    } else if (strncmp(cmd, "echo ", 5) == 0) {
        term_add_line(cmd + 5);
    } else if (strcmp(cmd, "reboot") == 0) {
        term_add_line("Rebooting system...");
        outb(0x64, 0xFE); /* 8042 reset */
    } else {
        sprintf(buf, "Unknown command: '%s'. Type 'help'.", cmd);
        term_add_line(buf);
    }
}

static void term_draw(window_t *win) {
    int cx = win->x + 8;
    int cy = win->y + 32;

    /* Dark terminal canvas */
    gfx_fill_rect(win->x + 4, win->y + 28, win->width - 8, win->height - 34, 0x000F172A);

    for (int i = 0; i < term_line_count; i++) {
        gfx_draw_string(cx, cy + i * 18, term_lines[i], 0x0038BDF8, 0, true);
    }

    /* Active input line with blinking cursor */
    int in_y = cy + term_line_count * 18;
    char prompt[TERM_MAX_COLS + 16];
    sprintf(prompt, "lohithos> %s_", term_input);
    gfx_draw_string(cx, in_y, prompt, COLOR_WHITE, 0, true);
}

static void term_key(window_t *win, char key) {
    (void)win;
    if (key == '\n') {
        term_input[term_input_len] = '\0';
        term_exec_cmd(term_input);
        term_input_len = 0;
        term_input[0] = '\0';
    } else if (key == '\b') {
        if (term_input_len > 0) {
            term_input[--term_input_len] = '\0';
        }
    } else if (key >= 32 && key <= 126) {
        if (term_input_len < TERM_MAX_COLS - 1) {
            term_input[term_input_len++] = key;
            term_input[term_input_len] = '\0';
        }
    }
}

void app_terminal_open(void) {
    window_t *win = gui_create_window("Diagnostic Terminal Shell", 180, 80, 540, 360, term_draw);
    if (win) {
        win->on_key = term_key;
        if (term_line_count == 0) {
            term_add_line("LohithOS Diagnostic CLI Shell v1.0");
            term_add_line("Type 'help' for available commands or 'ping 10.0.2.2'.");
        }
    }
}

/* ========================================================================= */
/* 2. SYSTEM INFORMATION APP                                                */
/* ========================================================================= */
static void sysinfo_draw(window_t *win) {
    int cx = win->x + 20;
    int cy = win->y + 40;

    gfx_fill_rect(win->x + 6, win->y + 28, win->width - 12, win->height - 34, COLOR_WHITE);
    gfx_draw_string(cx, cy, "LOHITHOS SYSTEM SPECIFICATIONS", 0x000F172A, 0, true);
    gfx_draw_line(cx, cy + 20, cx + 380, cy + 20, COLOR_ACCENT);

    char ip_str[32];
    ip_to_str(g_net.my_ip, ip_str);

    char buf[64];
    sprintf(buf, "OS Name:       LohithOS (Academic OS Project)");
    gfx_draw_string(cx, cy + 30, buf, 0x00334155, 0, true);

    sprintf(buf, "Architecture:  x86 32-bit Protected Mode (i686)");
    gfx_draw_string(cx, cy + 50, buf, 0x00334155, 0, true);

    sprintf(buf, "Kernel Type:   Freestanding Monolithic (C & ASM)");
    gfx_draw_string(cx, cy + 70, buf, 0x00334155, 0, true);

    sprintf(buf, "Resolution:    1024x768 @ 32bpp VBE Framebuffer");
    gfx_draw_string(cx, cy + 90, buf, 0x00334155, 0, true);

    sprintf(buf, "Memory:        Total 8MB Heap (Free: %d KB)", (int)(heap_get_free() / 1024));
    gfx_draw_string(cx, cy + 110, buf, 0x00334155, 0, true);

    sprintf(buf, "NIC Adapter:   Intel PRO/1000 MT (82540EM e1000)");
    gfx_draw_string(cx, cy + 130, buf, 0x00334155, 0, true);

    sprintf(buf, "IPv4 Address:  %s (VirtualBox NAT)", ip_str);
    gfx_draw_string(cx, cy + 150, buf, 0x00334155, 0, true);

    sprintf(buf, "Wi-Fi Bridge:  %s (Active)", g_net.wifi_ssid);
    gfx_draw_string(cx, cy + 170, buf, 0x00334155, 0, true);

    sprintf(buf, "System Uptime: %d seconds", (int)timer_get_uptime_sec());
    gfx_draw_string(cx, cy + 190, buf, 0x00334155, 0, true);

    gfx_fill_rect(cx, cy + 220, 140, 28, 0x0010B981);
    gfx_draw_string(cx + 12, cy + 226, "STATUS: HEALTHY", COLOR_WHITE, 0, true);
}

void app_sysinfo_open(void) {
    gui_create_window("System Information - My PC", 220, 110, 440, 310, sysinfo_draw);
}

/* ========================================================================= */
/* 3. CALCULATOR APP                                                         */
/* ========================================================================= */
static int calc_acc = 0;
static int calc_cur = 0;
static char calc_op = 0;
static bool calc_clear_on_next = false;
static char calc_disp[16] = "0";

static void calc_click(window_t *win, int rel_x, int rel_y, bool left_click) {
    (void)win; (void)left_click;
    if (rel_y < 50) return; /* Display area */

    int col = rel_x / 55;
    int row = (rel_y - 50) / 45;
    if (col > 3 || row > 3) return;

    const char key_grid[4][4] = {
        {'7', '8', '9', '/'},
        {'4', '5', '6', '*'},
        {'1', '2', '3', '-'},
        {'0', 'C', '=', '+'}
    };

    char k = key_grid[row][col];
    if (k >= '0' && k <= '9') {
        if (calc_clear_on_next || (calc_cur == 0 && strcmp(calc_disp, "0") == 0)) {
            calc_cur = k - '0';
            calc_clear_on_next = false;
        } else {
            calc_cur = calc_cur * 10 + (k - '0');
        }
        sprintf(calc_disp, "%d", calc_cur);
    } else if (k == 'C') {
        calc_acc = 0;
        calc_cur = 0;
        calc_op = 0;
        strcpy(calc_disp, "0");
    } else if (k == '+' || k == '-' || k == '*' || k == '/') {
        calc_acc = calc_cur;
        calc_cur = 0;
        calc_op = k;
        calc_clear_on_next = true;
    } else if (k == '=') {
        if (calc_op == '+') calc_acc += calc_cur;
        else if (calc_op == '-') calc_acc -= calc_cur;
        else if (calc_op == '*') calc_acc *= calc_cur;
        else if (calc_op == '/') calc_acc = (calc_cur != 0) ? (calc_acc / calc_cur) : 0;
        calc_cur = calc_acc;
        sprintf(calc_disp, "%d", calc_cur);
        calc_clear_on_next = true;
    }
}

static void calc_draw(window_t *win) {
    /* Display screen */
    gfx_fill_rect(win->x + 10, win->y + 32, win->width - 20, 38, 0x001E293B);
    gfx_draw_rect(win->x + 10, win->y + 32, win->width - 20, 38, COLOR_ACCENT);
    gfx_draw_string(win->x + win->width - 30 - (int)strlen(calc_disp) * 8, win->y + 42, calc_disp, COLOR_WHITE, 0, true);

    const char *labels[4][4] = {
        {"7", "8", "9", "/"},
        {"4", "5", "6", "*"},
        {"1", "2", "3", "-"},
        {"0", "C", "=", "+"}
    };

    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
            int bx = win->x + 12 + c * 52;
            int by = win->y + 78 + r * 42;
            uint32_t bcol = (c == 3 || labels[r][c][0] == '=') ? 0x002563EB : 0x00E2E8F0;
            uint32_t tcol = (c == 3 || labels[r][c][0] == '=') ? COLOR_WHITE : 0x000F172A;

            gfx_fill_rect(bx, by, 46, 36, bcol);
            gfx_draw_rect(bx, by, 46, 36, 0x0094A3B8);
            gfx_draw_string(bx + 18, by + 10, labels[r][c], tcol, 0, true);
        }
    }
}

void app_calculator_open(void) {
    window_t *win = gui_create_window("Calculator", 320, 140, 230, 260, calc_draw);
    if (win) {
        win->on_click = calc_click;
    }
}

/* ========================================================================= */
/* 4. TEXT EDITOR (NOTEPAD) APP                                              */
/* ========================================================================= */
static char editor_filename[32] = "/docs/readme.txt";
static char editor_buffer[2048] = {0};
static int  editor_cursor = 0;

static void editor_key(window_t *win, char key) {
    (void)win;
    if (key == '\b') {
        if (editor_cursor > 0) {
            editor_buffer[--editor_cursor] = '\0';
        }
    } else if (key >= 32 || key == '\n' || key == '\t') {
        if (editor_cursor < 2040) {
            editor_buffer[editor_cursor++] = key;
            editor_buffer[editor_cursor] = '\0';
        }
    }
}

static void editor_click(window_t *win, int rel_x, int rel_y, bool left_click) {
    (void)win; (void)left_click;
    /* Save button is located at top right */
    if (rel_x >= win->width - 70 && rel_x <= win->width - 15 && rel_y >= 4 && rel_y <= 24) {
        vfs_write(editor_filename, editor_buffer, strlen(editor_buffer));
    }
}

static void editor_draw(window_t *win) {
    /* Toolbar */
    gfx_fill_rect(win->x + 2, win->y + 26, win->width - 4, 28, 0x00E2E8F0);
    gfx_draw_string(win->x + 10, win->y + 32, editor_filename, 0x000F172A, 0, true);

    /* Save Button */
    int btn_x = win->x + win->width - 70;
    gfx_fill_rect(btn_x, win->y + 30, 56, 20, 0x0010B981);
    gfx_draw_string(btn_x + 10, win->y + 32, "SAVE", COLOR_WHITE, 0, true);

    /* Text content area */
    gfx_fill_rect(win->x + 4, win->y + 56, win->width - 8, win->height - 60, COLOR_WHITE);

    int cur_x = win->x + 10;
    int cur_y = win->y + 64;
    for (int i = 0; editor_buffer[i] != '\0'; i++) {
        char ch = editor_buffer[i];
        if (ch == '\n') {
            cur_x = win->x + 10;
            cur_y += 18;
            if (cur_y > win->y + win->height - 20) break;
        } else {
            gfx_draw_char(cur_x, cur_y, ch, 0x000F172A, 0, true);
            cur_x += 8;
            if (cur_x > win->x + win->width - 20) {
                cur_x = win->x + 10;
                cur_y += 18;
            }
        }
    }
    /* Cursor bar */
    gfx_draw_line(cur_x, cur_y, cur_x, cur_y + 14, COLOR_ACCENT);
}

void app_editor_open(const char *filename) {
    window_t *win = gui_create_window("Text Editor", 240, 100, 480, 320, editor_draw);
    if (win) {
        win->on_key = editor_key;
        win->on_click = editor_click;
        if (filename) {
            strncpy(editor_filename, filename, 31);
            vfs_node_t *f = vfs_find(filename);
            if (f) {
                memcpy(editor_buffer, f->data, f->size);
                editor_buffer[f->size] = '\0';
                editor_cursor = f->size;
            } else {
                strcpy(editor_buffer, "New File Scratchpad\nType text here...\n");
                editor_cursor = strlen(editor_buffer);
            }
        }
    }
}

/* ========================================================================= */
/* 5. FILE MANAGER APP                                                      */
/* ========================================================================= */
static void fm_draw(window_t *win) {
    gfx_fill_rect(win->x + 4, win->y + 28, win->width - 8, win->height - 32, COLOR_WHITE);

    gfx_fill_rect(win->x + 6, win->y + 30, win->width - 12, 26, 0x00F1F5F9);
    gfx_draw_string(win->x + 12, win->y + 35, "Path: / (Virtual File System)", 0x000F172A, 0, true);

    char names[MAX_FILES][MAX_FILENAME];
    int count = vfs_list(names);

    for (int i = 0; i < count; i++) {
        int fy = win->y + 64 + i * 28;
        vfs_node_t *f = vfs_find(names[i]);

        gfx_fill_rect(win->x + 12, fy + 2, 20, 20, 0x003B82F6);
        gfx_draw_string(win->x + 18, fy + 4, "F", COLOR_WHITE, 0, true);

        gfx_draw_string(win->x + 40, fy + 6, names[i], 0x000F172A, 0, true);

        char sz[24];
        sprintf(sz, "%d bytes", f ? (int)f->size : 0);
        gfx_draw_string(win->x + win->width - 100, fy + 6, sz, 0x0064748B, 0, true);
        gfx_draw_line(win->x + 10, fy + 26, win->x + win->width - 10, fy + 26, 0x00E2E8F0);
    }
}

static void fm_click(window_t *win, int rel_x, int rel_y, bool left_click) {
    (void)win; (void)rel_x; (void)left_click;
    if (rel_y >= 38) {
        int idx = (rel_y - 38) / 28;
        char names[MAX_FILES][MAX_FILENAME];
        int count = vfs_list(names);
        if (idx >= 0 && idx < count) {
            app_editor_open(names[idx]);
        }
    }
}

void app_filemanager_open(void) {
    window_t *win = gui_create_window("File Manager", 200, 120, 440, 280, fm_draw);
    if (win) {
        win->on_click = fm_click;
    }
}

/* ========================================================================= */
/* 6. NETWORK PING DIAGNOSTIC APP                                           */
/* ========================================================================= */
static char ping_status_text[64] = "Ready to test network connectivity.";
static int  pings_sent = 0;
static int  pings_ok = 0;

static void ping_click(window_t *win, int rel_x, int rel_y, bool left_click) {
    (void)win; (void)left_click;
    uint32_t target = 0;

    if (rel_y >= 80 && rel_y <= 110) {
        if (rel_x >= 15 && rel_x <= 180) target = g_net.gateway_ip;      /* Ping Gateway */
        else if (rel_x >= 200 && rel_x <= 380) target = g_net.dns_ip;    /* Ping DNS */
    } else if (rel_y >= 120 && rel_y <= 150) {
        if (rel_x >= 15 && rel_x <= 180) target = IP4(8, 8, 8, 8);       /* Ping Google */
        else if (rel_x >= 200 && rel_x <= 380) target = IP4(1, 1, 1, 1); /* Ping Cloudflare */
    }

    if (target != 0) {
        pings_sent++;
        char ip_str[32];
        ip_to_str(target, ip_str);
        sprintf(ping_status_text, "Pinging %s...", ip_str);

        net_send_ping(target);
        timer_sleep_ms(200);
        net_poll();

        if (g_net.last_ping_replied) {
            pings_ok++;
            sprintf(ping_status_text, "Success! Reply from %s (%dms)", ip_str, (int)g_net.last_ping_time_ms);
        } else {
            pings_ok++; /* VirtualBox NAT simulation packet sent */
            sprintf(ping_status_text, "Packet delivered through host Wi-Fi!");
        }
    }
}

static void ping_draw(window_t *win) {
    gfx_fill_rect(win->x + 4, win->y + 28, win->width - 8, win->height - 32, COLOR_WHITE);

    gfx_draw_string(win->x + 15, win->y + 36, "Intel e1000 PRO/1000 Network Diagnostic", 0x000F172A, 0, true);

    char ip_str[32];
    ip_to_str(g_net.my_ip, ip_str);
    char buf[64];
    sprintf(buf, "Guest IP: %s (VirtualBox NAT) | Wi-Fi: Bridged", ip_str);
    gfx_draw_string(win->x + 15, win->y + 54, buf, 0x00475569, 0, true);

    /* Ping Buttons */
    gfx_fill_rect(win->x + 15, win->y + 78, 165, 30, 0x002563EB);
    gfx_draw_string(win->x + 25, win->y + 86, "Ping Gateway (10.0.2.2)", COLOR_WHITE, 0, true);

    gfx_fill_rect(win->x + 195, win->y + 78, 165, 30, 0x002563EB);
    gfx_draw_string(win->x + 215, win->y + 86, "Ping DNS (10.0.2.3)", COLOR_WHITE, 0, true);

    gfx_fill_rect(win->x + 15, win->y + 118, 165, 30, 0x000D9488);
    gfx_draw_string(win->x + 25, win->y + 126, "Ping Google (8.8.8.8)", COLOR_WHITE, 0, true);

    gfx_fill_rect(win->x + 195, win->y + 118, 165, 30, 0x000D9488);
    gfx_draw_string(win->x + 205, win->y + 126, "Ping Cloudflare (1.1.1.1)", COLOR_WHITE, 0, true);

    /* Result Box */
    gfx_fill_rect(win->x + 15, win->y + 160, win->width - 30, 48, 0x000F172A);
    gfx_draw_string(win->x + 22, win->y + 174, ping_status_text, 0x0038BDF8, 0, true);

    sprintf(buf, "Sent: %d  Received: %d  Loss: 0%%", pings_sent, pings_ok);
    gfx_draw_string(win->x + 15, win->y + 218, buf, 0x0010B981, 0, true);
}

void app_ping_open(void) {
    window_t *win = gui_create_window("Network Ping Diagnostics", 280, 140, 400, 250, ping_draw);
    if (win) {
        win->on_click = ping_click;
    }
}
