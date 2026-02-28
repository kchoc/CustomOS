#include <machine/pmap.h>
#include <wm/window.h>
#include <wm/display.h>
#include <dev/video/vbe.h>
#include <vm/kmalloc.h>
#include <vm/vm_map.h>
#include <kern/terminal.h>

#include <kern/errno.h>
#include <inttypes.h>
#include <list.h>
#include <string.h>

static list_t window_list;
static uint32_t next_wid = 1;
static uint32_t next_z_order = 0;
static uint32_t* framebuffer = NULL;  // VBE linear framebuffer - set by wm_init()
static uint32_t fb_pitch_pixels = 0;  // Framebuffer pitch in pixels (not bytes!)
static bool wm_dirty = false;

void wm_init(void) {
    list_init(&window_list, 0);
    
    // Get framebuffer from VBE driver
    framebuffer = vbe_get_framebuffer_virtual();
    if (!framebuffer) {
        printf("Window Manager: WARNING - No framebuffer from VBE!\n");
        framebuffer = (uint32_t*)0xE0000000;  // Fallback
    }
    
    // Get pitch and convert from bytes to pixels (divide by 4 for 32-bit)
    uint16_t pitch_bytes = vbe_get_pitch();
    fb_pitch_pixels = pitch_bytes / 4;
    
    printf("Window Manager: OK (framebuffer=%p, pitch=%u pixels)\n", framebuffer, fb_pitch_pixels);
}

window_t* wm_create_window(uint32_t pid, const char* title, int x, int y, int width, int height) {
    // Validate dimensions
    if (width <= 0 || height <= 0 || width > SCREEN_WIDTH || height > SCREEN_HEIGHT) {
        return NULL;
    }
    
    window_t* win = kmalloc(sizeof(window_t));
    if (!win) return NULL;
    
    memset(win, 0, sizeof(window_t));
    
    win->wid = next_wid++;
    win->pid = pid;
    win->x = x;
    win->y = y;
    win->width = width;
    win->height = height;
    win->visible = 1;
    win->focused = 0;
    win->dirty = 1;
    win->z_order = next_z_order++;
    
    if (title) {
        strncpy(win->title, title, WINDOW_TITLE_MAX - 1);
        win->title[WINDOW_TITLE_MAX - 1] = '\0';
    }
    
    // Allocate backbuffer - allocate physical pages and map to kernel (32-bit RGBA)
    win->buffer_size = width * height * 4;  // 4 bytes per pixel
    
    // Allocate pages (4MB spacing per window to handle large windows)
    win->backbuffer = kvm_map(win->buffer_size, VM_PROT_READ | VM_PROT_WRITE | VM_PROT_NOCACHE, VM_REG_F_NONE);
    if (IS_ERR(win->backbuffer)) {
        kfree(win);
        return NULL;
    }
    
    // Add window to the list
    list_push_tail(&window_list, &win->node);

    // Mark WM as dirty to trigger screen update
    wm_dirty = true;

    return win;
}

void wm_destroy_window(uint32_t wid) {
    list_node_t* node = window_list.head;
    while (node) {
        window_t* win = (window_t*)((uint8_t*)node - offsetof(window_t, node));
        if (win->wid == wid) {
            list_remove(node);
            if (win->backbuffer) {
                kvm_unmap(win->backbuffer, win->buffer_size);
            }
            kfree(win);
            // Mark WM as dirty to trigger screen update
            wm_dirty = true;
            return;
        }
        node = node->next;
        if (node == window_list.head) break;
    }
}

window_t* wm_get_window(uint32_t wid) {
    list_node_t* node = window_list.head;
    while (node) {
        window_t* win = (window_t*)((uint8_t*)node - offsetof(window_t, node));
        if (win->wid == wid) return win;
        node = node->next;
        if (node == window_list.head) break;
    }
    return NULL;
}

window_t* wm_get_process_window(uint32_t pid) {
    list_node_t* node = window_list.head;
    while (node) {
        window_t* win = (window_t*)((uint8_t*)node - offsetof(window_t, node));
        if (win->pid == pid) return win;
        node = node->next;
        if (node == window_list.head) break;
    }
    return NULL;
}

void wm_set_window_title(uint32_t wid, const char* title) {
    window_t* win = wm_get_window(wid);
    if (win && title) {
        strncpy(win->title, title, WINDOW_TITLE_MAX - 1);
        win->title[WINDOW_TITLE_MAX - 1] = '\0';
        win->dirty = 1;
    }
}

void wm_move_window(uint32_t wid, int x, int y) {
    window_t* win = wm_get_window(wid);
    if (win) {
        win->x = x;
        win->y = y;
        win->dirty = 1;
    }

    wm_dirty = true;
}

void wm_resize_window(uint32_t wid, int width, int height) {
    window_t* win = wm_get_window(wid);
    if (!win) return;
    
    if (width <= 0 || height <= 0 || width > SCREEN_WIDTH || height > SCREEN_HEIGHT) {
        return;
    }
    
    size_t new_size = width * height;
    
    // Free old buffer pages
    kvm_unmap(win->backbuffer, win->buffer_size);
    
    // Allocate new physical pages
    win->backbuffer = kvm_map(new_size, VM_PROT_READ | VM_PROT_WRITE | VM_PROT_NOCACHE, VM_REG_F_NONE);
    if (IS_ERR(win->backbuffer)) {
        win->backbuffer = NULL;
        return;
    }
    
    win->buffer_size = new_size;
    win->width = width;
    win->height = height;
    win->dirty = 1;
    
    memset(win->backbuffer, 0, win->buffer_size);
    wm_dirty = true;
}

void wm_show_window(uint32_t wid) {
    window_t* win = wm_get_window(wid);
    if (win) {
        win->visible = 1;
        win->dirty = 1;
    }
    wm_dirty = true;
}

void wm_hide_window(uint32_t wid) {
    window_t* win = wm_get_window(wid);
    if (win) {
        win->visible = 0;
        win->dirty = 1;
    }
    wm_dirty = true;
}

void wm_focus_window(uint32_t wid) {
    // Unfocus all windows
    list_node_t* node = window_list.head;
    while (node) {
        window_t* win = (window_t*)((uint8_t*)node - offsetof(window_t, node));
        win->focused = 0;
        node = node->next;
        if (node == window_list.head) break;
    }
    
    // Focus the requested window
    window_t* win = wm_get_window(wid);
    if (win) {
        win->focused = 1;
        win->z_order = next_z_order++;
        win->dirty = 1;
    }
    wm_dirty = true;
}

void wm_mark_dirty(uint32_t wid) {
    window_t* win = wm_get_window(wid);
    if (win) win->dirty = 1;
}

uint32_t* wm_get_window_buffer(uint32_t wid) {
    window_t* win = wm_get_window(wid);
    return win ? win->backbuffer : NULL;
}

size_t wm_get_window_buffer_size(uint32_t wid) {
    window_t* win = wm_get_window(wid);
    return win ? win->buffer_size : 0;
}

/* Compositor - blit all visible windows to framebuffer in z-order */
void wm_composite(void) {
    if (!window_list.head) {
        return;
    }
        
    // Clear screen to black first (32-bit pixels)
    // Use pitch for proper scanline stride
    if (wm_dirty)
        memset(framebuffer, 0, fb_pitch_pixels * SCREEN_HEIGHT * 4);
            
    // Build sorted list of windows by z-order
    window_t* sorted[MAX_WINDOWS];
    int count = 0;
    
    list_node_t* node = window_list.head;
    while (node && count < MAX_WINDOWS) {
        window_t* win = (window_t*)((uint8_t*)node - offsetof(window_t, node));
        if (win->visible && (win->dirty || wm_dirty)) {
            sorted[count++] = win;
        }
        node = node->next;
        if (node == window_list.head) break;
    }
    
    // Simple bubble sort by z_order
    for (int i = 0; i < count - 1; i++) {
        for (int j = 0; j < count - i - 1; j++) {
            if (sorted[j]->z_order > sorted[j + 1]->z_order) {
                window_t* temp = sorted[j];
                sorted[j] = sorted[j + 1];
                sorted[j + 1] = temp;
            }
        }
    }
    
    // Composite windows in z-order (back to front)
    for (int i = 0; i < count; i++) {
        window_t* win = sorted[i];
        if (!win->dirty) {
            continue;
        }

        // Draw white border around window for debugging
        // Top border
        for (int x = 0; x < win->width + 2; x++) {
            int screen_x = win->x - 1 + x;
            int screen_y = win->y - 1;
            if (screen_x >= 0 && screen_x < SCREEN_WIDTH && screen_y >= 0 && screen_y < SCREEN_HEIGHT) {
                framebuffer[screen_y * fb_pitch_pixels + screen_x] = 0xFFFFFFFF;
            }
        }
        // Bottom border
        for (int x = 0; x < win->width + 2; x++) {
            int screen_x = win->x - 1 + x;
            int screen_y = win->y + win->height;
            if (screen_x >= 0 && screen_x < SCREEN_WIDTH && screen_y >= 0 && screen_y < SCREEN_HEIGHT) {
                framebuffer[screen_y * fb_pitch_pixels + screen_x] = 0xFFFFFFFF;
            }
        }
        // Left border
        for (int y = 0; y < win->height + 2; y++) {
            int screen_x = win->x - 1;
            int screen_y = win->y - 1 + y;
            if (screen_x >= 0 && screen_x < SCREEN_WIDTH && screen_y >= 0 && screen_y < SCREEN_HEIGHT) {
                framebuffer[screen_y * fb_pitch_pixels + screen_x] = 0xFFFFFFFF;
            }
        }
        // Right border
        for (int y = 0; y < win->height + 2; y++) {
            int screen_x = win->x + win->width;
            int screen_y = win->y - 1 + y;
            if (screen_x >= 0 && screen_x < SCREEN_WIDTH && screen_y >= 0 && screen_y < SCREEN_HEIGHT) {
                framebuffer[screen_y * fb_pitch_pixels + screen_x] = 0xFFFFFFFF;
            }
        }

        // Blit window backbuffer to framebuffer
        for (int y = 0; y < win->height; y++) {
            int screen_y = win->y + y;
            if (screen_y < 0 || screen_y >= SCREEN_HEIGHT) continue;
            
            int screen_x_start = win->x;
            int screen_x_end = win->x + win->width;
            if (screen_x_start < 0) screen_x_start = 0;
            if (screen_x_end > SCREEN_WIDTH) screen_x_end = SCREEN_WIDTH;
            memcpy(&framebuffer[screen_y * fb_pitch_pixels + screen_x_start],
                   &win->backbuffer[y * win->width + (screen_x_start - win->x)],
                     (screen_x_end - screen_x_start) * 4);
        }
        
        win->dirty = 0;
    }
}

void wm_update_region(int x, int y, int width, int height) {
    // Mark all windows in this region as dirty
    list_node_t* node = window_list.head;
    while (node) {
        window_t* win = (window_t*)((uint8_t*)node - offsetof(window_t, node));
        
        // Check if window intersects with region
        if (win->visible &&
            win->x < x + width && win->x + win->width > x &&
            win->y < y + height && win->y + win->height > y) {
            win->dirty = 1;
        }
        
        node = node->next;
        if (node == window_list.head) break;
    }
    
    wm_composite();
}
