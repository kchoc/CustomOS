#include "graphics.h"
#include "syscalls.h"

void _start() {
    // Create first window
    int win1 = win_create("Red Window", 20, 20, 150, 100);
    if (win1 < 0) {
        print("Failed to create window 1\n");
        exit(1);
    }
    
    gfx_init_window(win1, 150, 100);
    gfx_clear_screen(COLOR_RED);
    gfx_draw_string(10, 10, "Window 1", COLOR_WHITE, COLOR_RED);
    gfx_draw_rect(5, 5, 140, 90, COLOR_YELLOW);
    gfx_flush();
    
    // Create second window (overlapping)
    int win2 = win_create("Blue Window", 80, 60, 150, 100);
    if (win2 < 0) {
        print("Failed to create window 2\n");
        win_destroy(win1);
        exit(1);
    }
    
    gfx_init_window(win2, 150, 100);
    gfx_clear_screen(COLOR_BLUE);
    gfx_draw_string(10, 10, "Window 2", COLOR_YELLOW, COLOR_BLUE);
    gfx_draw_string(10, 20, "Overlapping!", COLOR_WHITE, 255);
    
    // Draw some shapes
    gfx_fill_rect(20, 40, 30, 30, COLOR_GREEN);
    gfx_fill_rect(60, 40, 30, 30, COLOR_MAGENTA);
    gfx_fill_rect(100, 40, 30, 30, COLOR_CYAN);
    
    gfx_flush();
    
    // Create third window
    int win3 = win_create("Pattern", 140, 100, 160, 90);
    if (win3 >= 0) {
        gfx_init_window(win3, 160, 90);
        gfx_clear_screen(COLOR_BLACK);
        
        // Draw a checkerboard pattern
        for (int y = 0; y < 90; y += 10) {
            for (int x = 0; x < 160; x += 10) {
                uint8_t color = ((x/10 + y/10) % 2) ? COLOR_WHITE : COLOR_DARK_GREY;
                gfx_fill_rect(x, y, 10, 10, color);
            }
        }
        
        gfx_draw_string(40, 40, "Checkerboard", COLOR_RED, 255);
        gfx_flush();
    }
    
    print("Windows created! Check graphics mode display.\n");
    print("Three windows should be visible with different content.\n");
    
    // Keep windows open
    for (volatile int i = 0; i < 50000000; i++);
    
    // Cleanup
    if (win3 >= 0) win_destroy(win3);
    win_destroy(win2);
    win_destroy(win1);
    
    print("Windows destroyed.\n");
    exit(0);
}
