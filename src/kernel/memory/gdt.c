#include "kernel/memory/gdt.h"
#include "kernel/terminal.h"

#define GDT_ENTRIES 5
struct gdt_entry gdt[GDT_ENTRIES];
struct gdt_ptr gp;

void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t grandularity) {
    gdt[num].base_low = (base & 0xFFFF);
    gdt[num].base_middle = (base >> 16) & 0xFF;
    gdt[num].base_high = (base >> 24) & 0xFF;
    gdt[num].limit_low = (limit & 0xFFFF);
    gdt[num].granularity = ((limit >> 16) & 0x0F) | (grandularity & 0xF0);
    gdt[num].access = access;
}

void load_gdt(struct gdt_ptr *gdt_desc) {
    asm volatile (
        "lgdt (%0)"  // Load GDTR with the address stored in gdt_desc
        :            // No outputs
        : "r" (gdt_desc) // Input: gdt_desc pointer
        : "memory"   // Tell the compiler memory is modified
    );
}

// Initialize GDT
void initialize_gdt(void) {
    gp.limit = (sizeof(struct gdt_entry) * GDT_ENTRIES) - 1;
    gp.base = (uint32_t)&gdt;

    gdt_set_gate(0, 0, 0, 0, 0);                // Null segment             e.g. 0x0000 0000 0000 0000
    gdt_set_gate(1, 0, 0xFFFFF, 0x9A, 0xCF); // Code segment             e.g. 0x00AF 9A00 0000 FFFF
    gdt_set_gate(2, 0, 0xFFFFF, 0x92, 0xCF); // Data segment             e.g. 0x00CF 9200 0000 FFFF
    gdt_set_gate(3, 0, 0xFFFFF, 0xFA, 0xCF); // User mode code segment
    gdt_set_gate(4, 0, 0xFFFFF, 0xF2, 0xCF); // User mode data segment

    load_gdt(&gp);
}