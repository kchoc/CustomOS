#include "graphics.h"
#include "syscalls.h"

void _start() {
    // Initialize graphics library (maps framebuffer)
    gfx_init();
    
    // Clear screen to black
    gfx_clear_screen(COLOR_BLACK);
    
    // Draw some colored rectangles
    gfx_fill_rect(10, 10, 50, 50, COLOR_RED);
    gfx_fill_rect(70, 10, 50, 50, COLOR_GREEN);
    gfx_fill_rect(130, 10, 50, 50, COLOR_BLUE);
    
    // Draw outline rectangles
    gfx_draw_rect(10, 70, 50, 50, COLOR_YELLOW);
    gfx_draw_rect(70, 70, 50, 50, COLOR_CYAN);
    gfx_draw_rect(130, 70, 50, 50, COLOR_MAGENTA);
    
    // Draw some lines
    gfx_draw_line(10, 130, 180, 130, COLOR_WHITE);
    gfx_draw_line(10, 130, 95, 180, COLOR_LIGHT_RED);
    gfx_draw_line(180, 130, 95, 180, COLOR_LIGHT_BLUE);
    
    // Draw some text
    gfx_draw_string(10, 10, "Graphics Test", COLOR_WHITE, COLOR_BLACK);
    gfx_draw_string(10, 20, "Hello from userland!", COLOR_YELLOW, 255);
    
    // Exit
    exit(0);
}
