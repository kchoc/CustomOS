#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "kernel/types.h"

// Function declerations
void handle_keypress(uint8_t scancode);
char scancode_to_ascii(uint8_t scancode);

#endif // KEYBOARD_H