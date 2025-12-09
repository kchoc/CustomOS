#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "kernel/types.h"
#include "types/list.h"

// Function declerations
void handle_keypress(uint8_t scancode);
char scancode_to_ascii(uint8_t scancode);

// Keyboard buffer access
int keyboard_has_input(void);
char keyboard_getchar(void);

// Wait queue for stdin blocking
list_t* get_stdin_wait_queue(void);

#endif // KEYBOARD_H