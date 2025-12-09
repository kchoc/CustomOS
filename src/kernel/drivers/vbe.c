#include "kernel/drivers/vbe.h"
#include "kernel/multiboot.h"
#include "kernel/drivers/port_io.h"
#include "kernel/memory/mm.h"
#include "kernel/memory/vm.h"
#include "kernel/terminal.h"
#include "kernel/wm/display.h"
#include "types/string.h"

static vbe_mode_info_t current_mode_info;
static uint32_t* framebuffer_virtual = (uint32_t*)0xE0000000; // Kernel virtual address for FB
static int vbe_initialized = 0;

int vbe_init(void) {
    multiboot_info_t* mbi = get_multiboot_info();
    if (!mbi) {
        printf("VBE: No multiboot info\n");
        return -1;
    }
    
    // Check if framebuffer info is available (flag bit 12)
    printf("VBE: Multiboot flags = 0x%x\n", mbi->flags);
    if (!(mbi->flags & (1 << 12))) {
        printf("VBE: No framebuffer info from bootloader (bit 12 not set)\n");
        return -1;
    }
    
    // Get framebuffer info from multiboot
    current_mode_info.framebuffer = (uint32_t)mbi->framebuffer_addr;
    current_mode_info.pitch = mbi->framebuffer_pitch;
    current_mode_info.width = mbi->framebuffer_width;
    current_mode_info.height = mbi->framebuffer_height;
    current_mode_info.bpp = mbi->framebuffer_bpp;
    
    printf("VBE: Physical FB at 0x%x\n", current_mode_info.framebuffer);
    
    // Calculate framebuffer size
    uint32_t fb_size = current_mode_info.pitch * current_mode_info.height;
    
    // Map physical framebuffer to 0xE0000000 virtual
    printf("VBE: Mapping framebuffer 0x%x -> 0xE0000000 (size=%d KB)\n", 
           current_mode_info.framebuffer, fb_size / 1024);
    
    vmm_map(framebuffer_virtual, current_mode_info.framebuffer, fb_size,
            VM_PROT_READWRITE | VM_PROT_NOCACHE, VM_MAP_PHYS | VM_MAP_FORCE);
    
    vbe_initialized = 1;
    
    printf("VBE: %dx%dx%d (pitch=%d)\n",
           current_mode_info.width,
           current_mode_info.height,
           current_mode_info.bpp,
           current_mode_info.pitch);
    
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
