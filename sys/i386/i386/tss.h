#ifndef x86_I386_TSS_H
#define x86_I386_TSS_H

#include <inttypes.h>
#include <machine/segment.h>

typedef struct gdt gdt_t;

typedef struct tss {
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
} __attribute__((packed)) tss_t;

void write_tss(seg_desc_t *gdt, int num, tss_t *tss, uint32_t ss0, uint32_t esp0);

extern void load_tss(uint16_t tss_selector);

#endif // x86_I386_TSS_H
