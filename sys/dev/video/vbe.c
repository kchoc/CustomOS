#include "vbe.h"
#include <dev/port/port_io.h>
#include <vm/vm_map.h>
#include <kern/terminal.h>
#include <string.h>

static vbe_mode_info_t current_mode_info;
static uint32_t* framebuffer_virtual = (uint32_t*)0xE0000000; // Kernel virtual address for FB
static int vbe_initialized = 0;

int vbe_init(void) {
    // multiboot_info_t* mbi = get_multiboot_info();
    // if (!mbi) {
    //     printf("VBE: No multiboot info\n");
    //     return -1;
    // }
    
    // // Check if framebuffer info is available (flag bit 12)
    // if (!(mbi->flags & (1 << 12))) {
    //     printf("VBE: No framebuffer info from bootloader (bit 12 not set)\n");
    //     return -1;
    // }
    
    // // Get framebuffer info from multiboot
    // current_mode_info.framebuffer = (uint32_t)mbi->framebuffer_addr;
    // current_mode_info.pitch = mbi->framebuffer_pitch;
    // current_mode_info.width = mbi->framebuffer_width;
    // current_mode_info.height = mbi->framebuffer_height;
    // current_mode_info.bpp = mbi->framebuffer_bpp;
    
    // Calculate framebuffer size
    uint32_t fb_size = current_mode_info.pitch * current_mode_info.height;
    
    // Map physical framebuffer to 0xE0000000 virtual
    vm_map_region(CURRENT_VM_SPACE, (uintptr_t)framebuffer_virtual, current_mode_info.framebuffer, fb_size,
            VM_PROT_READ | VM_PROT_NOCACHE, VM_MAP_PHYS);
    
    vbe_initialized = 1;
    
    return 0;
}

int vbe_set_mode(uint16_t width, uint16_t height, uint8_t bpp) {
    // This would require real mode BIOS calls (INT 0x10, AX=0x4F02)
    // For now, we just use what the bootloader gave us
    printf("VBE: Mode switching not implemented (using bootloader mode)\n");
    return -1;
}

uint32_t vbe_get_framebuffer(void) {
    return current_mode_info.framebuffer;
}

uint32_t* vbe_get_framebuffer_virtual(void) {
    return framebuffer_virtual;
}

uint16_t vbe_get_pitch(void) {
    return current_mode_info.pitch;
}

uint16_t vbe_get_width(void) {
    return current_mode_info.width;
}

uint16_t vbe_get_height(void) {
    return current_mode_info.height;
}

uint8_t vbe_get_bpp(void) {
    return current_mode_info.bpp;
}
