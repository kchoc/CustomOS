#ifndef GDT_H
#define GDT_H

#include <stdint.h>

// GDT structures
struct gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_middle;
    uint8_t access;
    uint8_t granularity;
    uint8_t base_high;
} __attribute__((packed));

struct gdt_ptr {
    uint16_t size;
    uintptr_t offset;
} __attribute__((packed));

void gdt_init();

extern void load_gdt(void*);

#endif // GDT_H