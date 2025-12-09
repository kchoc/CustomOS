#include "kernel/multiboot.h"
#include "kernel/terminal.h"
#include "types/string.h"

multiboot_info_t* get_multiboot_info(void) {
    printf("Multiboot: magic=0x%x, ptr=0x%x\n", multiboot_magic, multiboot_info_ptr);
    
    if (multiboot_magic != MULTIBOOT_MAGIC) {
        printf("Invalid multiboot magic: 0x%x (expected 0x%x)\n", 
               multiboot_magic, MULTIBOOT_MAGIC);
        return NULL;
    }
    
    // Convert physical address to virtual (add kernel offset)
    multiboot_info_t* mbi = (multiboot_info_t*)(multiboot_info_ptr + 0xC0000000);
    printf("Multiboot: info at virtual 0x%p\n", mbi);
    return mbi;
}
