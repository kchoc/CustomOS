#ifndef GDT_H
#define GDT_H

#include "kernel/types.h"

#define GDT_ENTRIES 6

typedef struct gdt_entry gdt_t;
typedef struct gdt_ptr gdt_ptr_t;
typedef struct tss_entry tss_t;

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

struct tss_entry {
    uint32_t prev_tss;
    uint32_t esp0;       // Kernel stack pointer
    uint32_t ss0;        // Kernel stack segment
    uint32_t esp1;
    uint32_t ss1;
    uint32_t esp2;
    uint32_t ss2;
    uint32_t cr3;
    uint32_t eip;
    uint32_t eflags;
    uint32_t eax;
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;
    uint32_t esp;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;
    uint32_t es;
    uint32_t cs;
    uint32_t ss;
    uint32_t ds;
    uint32_t fs;
    uint32_t gs;
    uint32_t ldt;
    uint16_t trap;
    uint16_t iomap_base;
} __attribute__((packed));

void gdt_init_percpu(int, uint32_t);
void write_tss(gdt_t* gdt, int num, tss_t* tss, uint32_t ss0, uint32_t esp0);

extern void load_gdt(void* gdt_ptr);
extern void load_tss(uint16_t tss_selector);

#endif // GDT_H