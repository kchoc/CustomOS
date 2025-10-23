#include "kernel/descriptors/gdt.h"
#include "kernel/process/cpu.h"
#include "types/string.h"
#include "kernel/terminal.h"

struct gdt_ptr gp;
struct tss_entry tss;

void write_tss(gdt_t *gdt, int num, struct tss_entry *tss,
               uint32_t ss0, uint32_t esp0) {
    uint32_t base = (uint32_t)tss;
    uint32_t limit = sizeof(tss_t) - 1;

    gdt[num].base_low       = base & 0xFFFF;
    gdt[num].base_middle    = (base >> 16) & 0xFF;
    gdt[num].base_high      = (base >> 24) & 0xFF;
    gdt[num].limit_low      = limit & 0xFFFF;
    gdt[num].granularity    = (limit >> 16) & 0x0F;
    gdt[num].access         = 0x89; // Present, Ring 0, Type 9 (available 32-bit TSS)

    memset(tss, 0, sizeof(tss_t));
    tss->ss0 = ss0;
    tss->esp0 = esp0;
    tss->cs = 0x08; // Kernel mode code segment
    tss->ss = tss->ds = tss->es = tss->fs = tss->gs = 0x10; // Kernel mode data segment
    tss->iomap_base = sizeof(tss_t);
}

void gdt_init_percpu(int cpu_id, uint32_t kernel_stack_top)
{
    cpu_t* cpu = &cpus[cpu_id];

    // Copy base GDT entries (kernel/user segments same for all CPUs)
    if (cpu_id == 0) {
        // CPU0: build from scratch
        cpu->gdt[0] = (gdt_t){0};
        cpu->gdt[1] = (gdt_t){ .limit_low=0xFFFF, .access=0x9A, .granularity=0xCF, .base_low=0, .base_middle=0, .base_high=0 };
        cpu->gdt[2] = (gdt_t){ .limit_low=0xFFFF, .access=0x92, .granularity=0xCF, .base_low=0, .base_middle=0, .base_high=0 };
        cpu->gdt[3] = (gdt_t){ .limit_low=0xFFFF, .access=0xFA, .granularity=0xCF, .base_low=0, .base_middle=0, .base_high=0 };
        cpu->gdt[4] = (gdt_t){ .limit_low=0xFFFF, .access=0xF2, .granularity=0xCF, .base_low=0, .base_middle=0, .base_high=0 };
    } else {
        // For APs, copy BSP’s GDT template (first 5 entries)
        memcpy(cpu->gdt, cpus[0].gdt, sizeof(gdt_t) * 5);
    }

    // Initialize TSS for this CPU
    memset(&cpu->tss, 0, sizeof(tss_t));
    write_tss(cpu->gdt, 5, &cpu->tss, 0x10, kernel_stack_top);

    cpu->gdt_ptr.size   = sizeof(gdt_t) * GDT_ENTRIES - 1;
    cpu->gdt_ptr.offset = (uint32_t)cpu->gdt;

    // Load the GDT and TSS for this CPU
    load_gdt(&cpu->gdt_ptr);

    load_tss(0x28);  // selector for entry 5 (TSS)
}
