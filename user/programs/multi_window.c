#include "graphics.h"
#include "syscalls.h"

void _start() {
    print("Creating multi-window demo with contexts...\n");
    
    // Create three windows with different sizes
    int wid1 = win_create("Red Window", 20, 20, 180, 120);
    int wid2 = win_create("Blue Window", 90, 60, 200, 100);
    int wid3 = win_create("Green Window", 160, 100, 150, 80);
    
    if (wid1 < 0 || wid2 < 0 || wid3 < 0) {
        print("Failed to create windows\n");
        exit(1);
    }
    
    // Create separate contexts for each window
    gfx_context_t* ctx1 = gfx_create_context(wid1, 180, 120);
    gfx_context_t* ctx2 = gfx_create_context(wid2, 200, 100);
    gfx_context_t* ctx3 = gfx_create_context(wid3, 150, 80);
    
    // Draw to first window using context-explicit API
    gfx_clear_screen_ctx(ctx1, COLOR_RED);
    gfx_draw_string_ctx(ctx1, 10, 10, "Context Window 1", COLOR_WHITE, COLOR_RED);
    gfx_draw_string_ctx(ctx1, 10, 20, "Size: 180x120", COLOR_YELLOW, 255);
    gfx_draw_rect_ctx(ctx1, 5, 5, 170, 110, COLOR_WHITE);
    gfx_fill_rect_ctx(ctx1, 20, 40, 40, 40, COLOR_YELLOW);
    gfx_fill_rect_ctx(ctx1, 70, 40, 40, 40, COLOR_MAGENTA);
    gfx_fill_rect_ctx(ctx1, 120, 40, 40, 40, COLOR_CYAN);
    gfx_flush_ctx(ctx1);
    
    // Draw to second window
    gfx_clear_screen_ctx(ctx2, COLOR_BLUE);
    gfx_draw_string_ctx(ctx2, 10, 10, "Context Window 2", COLOR_YELLOW, COLOR_BLUE);
    gfx_draw_string_ctx(ctx2, 10, 20, "Size: 200x100", COLOR_WHITE, 255);
    
    // Draw diagonal lines
    for (int i = 0; i < 200; i += 20) {
        gfx_draw_line_ctx(ctx2, i, 0, i, 99, COLOR_LIGHT_BLUE);
    }
    for (int i = 0; i < 100; i += 10) {
        gfx_draw_line_ctx(ctx2, 0, i, 199, i, COLOR_LIGHT_CYAN);
    }
    
    gfx_flush_ctx(ctx2);
    
    // Draw to third window
    gfx_clear_screen_ctx(ctx3, COLOR_GREEN);
    gfx_draw_string_ctx(ctx3, 10, 10, "Context Win 3", COLOR_BLACK, COLOR_GREEN);
    gfx_draw_string_ctx(ctx3, 10, 20, "Size: 150x80", COLOR_WHITE, 255);
    
    // Draw checkerboard pattern
    for (int y = 30; y < 75; y += 5) {
        for (int x = 10; x < 140; x += 5) {
            uint8_t color = ((x/5 + y/5) % 2) ? COLOR_DARK_GREY : COLOR_LIGHT_GREY;
            gfx_fill_rect_ctx(ctx3, x, y, 5, 5, color);
        }
    }
    
    gfx_flush_ctx(ctx3);
    
    print("Three windows rendered using context-explicit API!\n");
    print("Each window has different dimensions.\n");
    
    // Keep windows open
    for (volatile int i = 0; i < 100000000; i++);
    
    // Cleanup
    gfx_destroy_context(ctx1);
    gfx_destroy_context(ctx2);
    gfx_destroy_context(ctx3);
    win_destroy(wid1);
    win_destroy(wid2);
    win_destroy(wid3);
    
    print("Demo complete!\n");
    exit(0);
}
