#include "graphics.h"
#include "stdio.h"
#include "syscalls.h"

#define TERM_WIDTH  1024
#define TERM_HEIGHT 600
#define TERM_COLS   (TERM_WIDTH / 8)
#define TERM_ROWS   (TERM_HEIGHT / 8)

static int cursor_x = 0;
static int cursor_y = 0;
static int wid = -1;

void term_putchar(char c) {
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
    } else if (c == '\r') {
        cursor_x = 0;
    } else if (c == '\b') {
        if (cursor_x > 0) {
            cursor_x--;
            // Erase the character
            gfx_fill_rect(cursor_x * 8, cursor_y * 8, 8, 8, COLOR_BLACK);
        }
    } else if (c >= 32 && c < 127) {
        gfx_draw_char(cursor_x * 8, cursor_y * 8, c, COLOR_WHITE, COLOR_BLACK);
        cursor_x++;
        
        if (cursor_x >= TERM_COLS) {
            cursor_x = 0;
            cursor_y++;
        }
    }
    
    // Scroll if needed
    if (cursor_y >= TERM_ROWS) {
        cursor_y = TERM_ROWS - 1;
        // TODO: Implement scrolling by copying screen contents up
        // For now, just clear and restart
        gfx_clear_screen(COLOR_BLACK);
        cursor_x = 0;
        cursor_y = 0;
    }
}

void term_write(const char* str) {
    while (*str) {
        term_putchar(*str++);
    }
    gfx_flush();
}

void _start() {
    print("Starting terminal...\n");
    
    // Create window for terminal
    wid = win_create("Terminal", 20, 10, TERM_WIDTH, TERM_HEIGHT);
    if (wid < 0) {
        print("Failed to create terminal window\n");
        exit(1);
    }
    
    // Initialize graphics
    gfx_init_window(wid, TERM_WIDTH, TERM_HEIGHT);
    gfx_clear_screen(COLOR_BLACK);
    
    // Welcome message
    term_write("CustomOS Terminal\n");
    term_write("Type text and press Enter\n");
    term_write("> ");
    gfx_flush();
    
    // Main input loop
    char input_line[256];
    int line_pos = 0;
    
    while (1) {
        int c = getchar();
        
        if (c == EOF) break;
        
        if (c == '\n') {
            term_putchar('\n');
            
            // Process command
            input_line[line_pos] = '\0';
            
            if (line_pos > 0) {
                term_write("You typed: ");
                term_write(input_line);
                term_putchar('\n');
                
                // Check for exit command
                if (input_line[0] == 'e' && input_line[1] == 'x' && 
                    input_line[2] == 'i' && input_line[3] == 't' && 
                    input_line[4] == '\0') {
                    term_write("Goodbye!\n");
                    gfx_flush();
                    break;
                }
            }
            
            line_pos = 0;
            term_write("> ");
            gfx_flush();
            
        } else if (c == '\b' || c == 127) {
            // Backspace
            if (line_pos > 0) {
                line_pos--;
                term_putchar('\b');
                gfx_flush();
            }
        } else if (c >= 32 && c < 127 && line_pos < 255) {
            // Printable character
            input_line[line_pos++] = (char)c;
            term_putchar((char)c);
            gfx_flush();
        }
    }
    
    // Cleanup
    win_destroy(wid);
    exit(0);
}
