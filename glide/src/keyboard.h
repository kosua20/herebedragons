#ifndef KEYBOARD_H
#define KEYBOARD_H

#define KEY_EXT_SHIFT 0x60
#define KEY_LEFT (KEY_EXT_SHIFT + 75)
#define KEY_RIGHT (KEY_EXT_SHIFT + 77)
#define KEY_UP (KEY_EXT_SHIFT + 72)
#define KEY_DOWN (KEY_EXT_SHIFT + 80)
#define KEY_R 19
#define KEY_W 17
#define KEY_S 31
#define KEY_ESC 1

void initKeyboard();

void cleanupKeyboard();

unsigned char isPressed(const int key);

#endif
