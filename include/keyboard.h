#ifndef _KEYBOARD_H_
#define _KEYBOARD_H_

#include "types.h"

#define KEY_ESC        27
#define KEY_BACKSPACE  '\b'
#define KEY_TAB        '\t'
#define KEY_ENTER      '\n'
#define KEY_SPACE      ' '
#define KEY_UP         0x80
#define KEY_DOWN       0x81
#define KEY_LEFT       0x82
#define KEY_RIGHT      0x83
#define KEY_F1         0x90
#define KEY_F2         0x91
#define KEY_F3         0x92
#define KEY_F4         0x93

void keyboard_init(void);
char kbd_getchar(void);
bool kbd_has_key(void);
bool kbd_is_shift(void);
bool kbd_is_ctrl(void);

#endif
