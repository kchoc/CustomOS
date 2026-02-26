#include "graphics.h"
#include "syscalls.h"

void _start() {
    print("Starting windowed graphics test...\n");

    // Create a window (200x150 at position 50,25)
    int wid = win_create("My Window", 50, 25, 200, 150);
    if (wid < 0) {
        print("Failed to create window\n");
        exit(1);
    }
    
    print("Window created successfully!\n");
    
    // Initialize graphics library with window buffer
    gfx_init_window(wid, 200, 150);
    
    print("Graphics initialized, starting animation...\n");
    
    // Animate the window with periodic updates
    for (int frame = 0; frame < 100; frame++) {
        // Clear window to blue
        gfx_clear_screen(COLOR_BLUE);
        
        // Draw animated rectangles (moving across)
        int offset = (frame * 2) % 150;
        gfx_fill_rect(offset, 10, 40, 40, COLOR_RED);
        gfx_fill_rect(10, 10 + offset/3, 40, 40, COLOR_GREEN);
        gfx_fill_rect(110, 10, 40, 40, COLOR_YELLOW);
        
        // Draw text
        gfx_draw_string(10, 60, "Hello Window!", COLOR_WHITE, COLOR_BLUE);
        gfx_draw_string(10, 70, "Frame: ", COLOR_YELLOW, COLOR_BLUE);
        
        // Draw some lines
        gfx_draw_line(10, 90, 180, 90, COLOR_WHITE);
        gfx_draw_line(10, 90, 95, 130, COLOR_CYAN);
        gfx_draw_line(180, 90, 95, 130, COLOR_MAGENTA);
        
        // Flush to screen periodically
        gfx_flush();
        
        // Small delay between frames
        for (volatile int i = 0; i < 5000000; i++);
    }
    
    print("Animation complete!\n");
    
    // Cleanup
    win_destroy(wid);
    print("Window destroyed, exiting...\n");
    exit(0);
}
