#ifndef WINDOW_H
#define WINDOW_H

#include "kernel/types.h"
#include "types/list.h"

#define MAX_WINDOWS 32
#define WINDOW_TITLE_MAX 64

typedef struct window {
    list_node_t node;           // For linking in window list
    
    uint32_t wid;               // Window ID
    uint32_t pid;               // Owner process ID
    
    char title[WINDOW_TITLE_MAX];
    
    int x, y;                   // Position on screen
    int width, height;          // Window dimensions
    
    uint32_t* backbuffer;       // Kernel virtual address of backbuffer (32-bit RGBA)
    size_t buffer_size;
    
    uint8_t visible;            // Is window visible?
    uint8_t focused;            // Is window focused?
    uint8_t dirty;              // Needs redraw?
    
    uint32_t z_order;           // Stacking order (higher = on top)
} window_t;

/* Window Manager Functions */
void wm_init(void);
window_t* wm_create_window(uint32_t pid, const char* title, int x, int y, int width, int height);
void wm_destroy_window(uint32_t wid);
window_t* wm_get_window(uint32_t wid);
window_t* wm_get_process_window(uint32_t pid);
void wm_set_window_title(uint32_t wid, const char* title);
void wm_move_window(uint32_t wid, int x, int y);
void wm_resize_window(uint32_t wid, int width, int height);
void wm_show_window(uint32_t wid);
void wm_hide_window(uint32_t wid);
void wm_focus_window(uint32_t wid);
void wm_mark_dirty(uint32_t wid);

/* Compositor */
void wm_composite(void);        // Composite all windows to framebuffer
void wm_update_region(int x, int y, int width, int height); // Update specific region

/* Window buffer access */
uint32_t* wm_get_window_buffer(uint32_t wid);
size_t wm_get_window_buffer_size(uint32_t wid);

#endif // WINDOW_H
