#ifndef DEV_KEYBOARD_H
#define DEV_KEYBOARD_H

#include <sys/tty.h>

#include <inttypes.h>
#include <list.h>

void keyboard_set_active_tty(tty_t* tty);

// Function declerations
void handle_keypress(uint8_t scancode);
char scancode_to_ascii(uint8_t scancode);

#endif // DEV_KEYBOARD_H
