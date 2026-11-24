#include "keyboard.h"
#include "isr.h"
#include "pic.h"

#define KBD_BUFFER_SIZE 256
static char kbd_buffer[KBD_BUFFER_SIZE];
static volatile int kbd_head = 0;
static volatile int kbd_tail = 0;

static bool shift_pressed = false;
static bool ctrl_pressed  = false;
static bool caps_lock     = false;

/* Scancode Set 1 standard mapping */
static const char kbd_us_lower[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, /* Ctrl */
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, /* Left Shift */
    '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',
    0, /* Right Shift */
    '*', 0, ' ', 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* F1-F10 */
    0, 0, /* NumLock, ScrollLock */
    0, 0, 0, '-', 0, 0, 0, '+', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static const char kbd_us_upper[128] = {
    0,  27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0,
    'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0,
    '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?',
    0,
    '*', 0, ' ', 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0,
    0, 0, 0, '-', 0, 0, 0, '+', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static void keyboard_callback(registers_t *regs) {
    (void)regs;
    uint8_t scancode = inb(0x60);

    /* Extended scancodes check */
    if (scancode == 0xE0) {
        return; /* Will read next byte */
    }

    /* Key release */
    if (scancode & 0x80) {
        uint8_t released = scancode & 0x7F;
        if (released == 0x2A || released == 0x36) {
            shift_pressed = false;
        } else if (released == 0x1D) {
            ctrl_pressed = false;
        }
        return;
    }

    /* Key press */
    if (scancode == 0x2A || scancode == 0x36) {
        shift_pressed = true;
        return;
    }
    if (scancode == 0x1D) {
        ctrl_pressed = true;
        return;
    }
    if (scancode == 0x3A) {
        caps_lock = !caps_lock;
        return;
    }

    /* Arrow keys */
    char ch = 0;
    if (scancode == 0x48) ch = (char)KEY_UP;
    else if (scancode == 0x50) ch = (char)KEY_DOWN;
    else if (scancode == 0x4B) ch = (char)KEY_LEFT;
    else if (scancode == 0x4D) ch = (char)KEY_RIGHT;
    else {
        bool shift = shift_pressed ^ caps_lock;
        if (shift) {
            ch = kbd_us_upper[scancode];
        } else {
            ch = kbd_us_lower[scancode];
        }
    }

    if (ch != 0) {
        int next = (kbd_head + 1) % KBD_BUFFER_SIZE;
        if (next != kbd_tail) {
            kbd_buffer[kbd_head] = ch;
            kbd_head = next;
        }
    }
}

void keyboard_init(void) {
    register_interrupt_handler(33, keyboard_callback);
    pic_clear_mask(1); /* Unmask IRQ1 */
}

bool kbd_has_key(void) {
    return (kbd_head != kbd_tail);
}

char kbd_getchar(void) {
    if (kbd_head == kbd_tail) return 0;
    char ch = kbd_buffer[kbd_tail];
    kbd_tail = (kbd_tail + 1) % KBD_BUFFER_SIZE;
    return ch;
}

bool kbd_is_shift(void) {
    return shift_pressed;
}

bool kbd_is_ctrl(void) {
    return ctrl_pressed;
}
