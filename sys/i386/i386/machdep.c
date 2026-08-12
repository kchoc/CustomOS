#include "machdep.h"
#include "gdt.h"
#include "idt.h"
#include "rsd.h"

#include <dev/pci/pci.h>
#include <dev/vga/vga.h>

#include <sys/pcpu.h>
#include <sys/root_bus.h>
#include <sys/tty.h>

#include <fs/vfs.h>

#include <vm/kmalloc.h>
#include <vm/layout.h>
#include <vm/vm_map.h>
#include <vm/vm_phys.h>
#include <vm/vm_space.h>

#include <x86/bios/bda.h>

#include <machine/bootinfo.h>
#include <machine/segment_i386.h>

#include <kern/elf.h>
#include <kern/errno.h>
#include <kern/lapic.h>
#include <kern/panic.h>
#include <kern/pit.h>
#include <kern/process.h>
#include <kern/syscalls.h>
#include <kern/system_init.h>
#include <kern/terminal.h>

void init386(void)
{
    region_desc_t r_gdt;

    terminal_init();

    // Initialise the gdt
    SET_SEGMENT_LIMIT(gdt[GCODE_SEL], 0xFFFFF);
    SET_SEGMENT_LIMIT(gdt[GDATA_SEL], 0xFFFFF);
    SET_SEGMENT_LIMIT(gdt[GUCODE_SEL], 0xFFFFF);
    SET_SEGMENT_LIMIT(gdt[GUDATA_SEL], 0xFFFFF);
    SET_SEGMENT_LIMIT(gdt[GUFS_SEL], 0xFFFFF);
    SET_SEGMENT_LIMIT(gdt[GUGS_SEL], 0xFFFFF);

    // Set up quick access segment for per-cpu data
    pcpu_t* pc = &pcpus[0];
    write_tss(&pc->tss, 0x10, KERNEL_STACK_TOP); // Set kernel stack segment and pointer in TSS
    SET_SEGMENT_LIMIT(gdt[GPRIV_SEL], 0xFFFFF);
    SET_SEGMENT_BASE(gdt[GPRIV_SEL], (uintptr_t)pc);
    SET_SEGMENT_BASE(gdt[GPROC0_SEL], (uintptr_t)&pc->tss);

    r_gdt.rd_limit = NGDT * sizeof(gdt[0]) - 1;
    r_gdt.rd_base  = (uintptr_t)&gdt;
    lgdt(&r_gdt);
    load_tss(GPROC0_SEL << 3);

    kmalloc_init((char*)KMALLOC_START, KMALLOC_SIZE);

    if (is_errno(vm_phys_init(bootinfo->memory_map, bootinfo->memory_map_length)))
        PANIC("Physical memory initialization: FAILED");
    vm_phys_dump_info();

    kvm_space_init();
    printf("Kernel VM space initialized.\n");

    syscalls_init();

    idt_init();

    lapic_init();

    pcpu_init(0);

    asm volatile("sti"); // Enable interrupts

    if (is_errno(load_bda()))
        PANIC("BDA initialization: FAILED");

    if (is_errno(rsdt_init()))
        PANIC("RSDT initialization: FAILED");

    vfs_init();

    drivers_init();
    root_bus.driver->bus_ops->enumerate(&root_bus);

    tty_init();
    vga_init();

    vfs_list_devices();

    system_init();

    // list_tasks();

    while (1)
        yield();
}
