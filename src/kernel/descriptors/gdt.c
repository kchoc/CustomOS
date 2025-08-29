#include "kernel/descriptors/gdt.h"
#include "string.h"

#define GDT_ENTRIES 6
struct gdt_entry gdt[GDT_ENTRIES];
struct gdt_ptr gp;
struct tss_entry tss;

void write_tss(int32_t num, uint32_t ss0, uint32_t esp0) {
    uint32_t base = (uint32_t)&tss;
    uint32_t limit = sizeof(tss) - 1;

    gdt[num].base_low = base & 0xFFFF;
    gdt[num].base_middle = (base >> 16) & 0xFF;
    gdt[num].base_high = (base >> 24) & 0xFF;
    gdt[num].limit_low = limit & 0xFFFF;
    gdt[num].granularity = (limit >> 16) & 0x0F;
    gdt[num].access = 0xE9;

    memset(&tss, 0, sizeof(tss));
    tss.ss0 = ss0;
    tss.esp0 = esp0;
    tss.cs = 0x0B;
    tss.ss = tss.ds = tss.es = tss.fs = tss.gs = 0x13;
}

// Initialize GDT
void gdt_init() {
	gdt[0] = (struct gdt_entry){0}; // Null descriptor

    gdt[1] = (struct gdt_entry) {
        .limit_low = 0xFFFF,
        .base_low = 0x0000,
        .base_middle = 0x00,
        .access = 0x9A, // Present, Ring 0, Code, Read/Execute
        .granularity = 0xCF, // 4KB, 32-bit
        .base_high = 0x00
    };

    gdt[2] = (struct gdt_entry) {
        .limit_low = 0xFFFF,
        .base_low = 0x0000,
        .base_middle = 0x00,
        .access = 0x92, // Present, Ring 0, Data, Read/Write
        .granularity = 0xCF, // 4KB, 32-bit
        .base_high = 0x00
    };

    gdt[3] = (struct gdt_entry) {
        .limit_low = 0xFFFF,
        .base_low = 0x0000,
        .base_middle = 0x00,
        .access = 0xFA, // Present, Ring 3, Code, Read/Execute
        .granularity = 0xCF, // 4KB, 32-bit
        .base_high = 0x00
    };

    gdt[4] = (struct gdt_entry) {
        .limit_low = 0xFFFF,
        .base_low = 0x0000,
        .base_middle = 0x00,
        .access = 0xF2, // Present, Ring 3, Data, Read/Write
        .granularity = 0xCF, // 4KB, 32-bit
        .base_high = 0x00
    };

    write_tss(5, 0x10, 0x0);

    gp.size = sizeof(gdt) - 1;
    gp.offset = (uint32_t)&gdt;

    load_gdt(&gp);
    load_tss(0x28);
}
