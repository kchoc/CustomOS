#include "kernel/drivers/keyboard.h"
#include "kernel/drivers/vga.h"
#include "kernel/terminal.h"
#include "kernel/panic.h"
#include "kernel/process/process.h"

// Base keyboard map
static const char keyboard_map[128] = {
    0, 0 /* Esc */, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0 /* Ctrl */, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0 /* Left Shift */, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0 /* Right Shift */,
    '*', 0 /* Alt */, ' ', 0 /* Caps Lock */,
    0,0,0,0,0,0,0,0,0,0, // F1–F10
    0,0,0,0,0,'-',0,0,0,'+',
    0,0,0,0,0
};

// Shifted version of the keyboard map
static const char keyboard_shift_map[128] = {
    0, 0, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0 /* Ctrl */, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,
    '*', 0, ' ', 0,
    0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,'-',0,0,0,'+',
    0,0,0,0,0
};

// Key state bitmask
static uint8_t shift = 0;
static uint8_t ctrl = 0;

static uint64_t keys[2] = {0};

// Keyboard input circular buffer
#define KBD_BUFFER_SIZE 256
static char kbd_buffer[KBD_BUFFER_SIZE];
static volatile int kbd_read_pos = 0;
static volatile int kbd_write_pos = 0;

// Wait queue for processes blocked on stdin
static list_t stdin_wait_queue = {0};

// Convert scancode to ASCII, considering modifiers
char scancode_to_ascii(uint8_t scancode) {
    if (scancode >= 0x80) return 0;

    char base = (shift ? keyboard_shift_map : keyboard_map)[scancode];

    // Optionally, handle Ctrl combinations
    if (ctrl && base >= 'a' && base <= 'z')
        return base - 'a' + 1; // Ctrl+A = 0x01, Ctrl+B = 0x02, etc.

    return base;
}

// Handle key press/release
void handle_keypress(uint8_t scancode) {
    uint8_t is_release = scancode & 0x80 ? 1 : 0;
    uint8_t clean = scancode & 0x7F;

    keys[(scancode >> 6) & 0x01] ^= 1UL << (scancode & 0x3F);

    // Track modifier state
    switch (clean) {
        case 0x2A: // Left Shift
        case 0x36: // Right Shift
            shift = !is_release;
            return;
        case 0x1D: // Ctrl
            ctrl = !is_release;
            return;
        case 0x01: // ESC key
            if (!is_release) {
                vga_set_mode_text();
            }
            return;
    }

    if (is_release) return; // Ignore key releases for characters

    if (hasPanicOccurred) reboot_system();
    char c = scancode_to_ascii(scancode);
    
    // Add to keyboard buffer
    if (c != 0) {
        int next_pos = (kbd_write_pos + 1) % KBD_BUFFER_SIZE;
        if (next_pos != kbd_read_pos) {  // Don't overflow
            kbd_buffer[kbd_write_pos] = c;
            kbd_write_pos = next_pos;
            
            // Wake up any tasks blocked on stdin
            wake_up_queue(&stdin_wait_queue);
        }
    }
    
    terminal_input(scancode, c);
}

// Check if keyboard buffer has input
int keyboard_has_input(void) {
    return kbd_read_pos != kbd_write_pos;
}

// Get a character from keyboard buffer (non-blocking, returns 0 if empty)
char keyboard_getchar(void) {
    if (kbd_read_pos == kbd_write_pos) {
        return 0;
    }
    char c = kbd_buffer[kbd_read_pos];
    kbd_read_pos = (kbd_read_pos + 1) % KBD_BUFFER_SIZE;
    return c;
}

// Get the stdin wait queue (for blocking operations)
list_t* get_stdin_wait_queue(void) {
    return &stdin_wait_queue;
}
