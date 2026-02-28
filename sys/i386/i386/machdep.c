#include "machdep.h"
#include "idt.h"
#include "gdt.h"
#include <i386/bios/bda.h>
#include "vm/kmalloc.h"

#include <machine/bootinfo.h>
#include <machine/segment_i386.h>

#include <vm/vm_phys.h>
#include <vm/vm_space.h>
#include <vm/vm_map.h>
#include <vm/layout.h>

#include <kern/pcpu.h>
#include <kern/terminal.h>
#include <kern/panic.h>

#include <kern/errno.h>

void init386(void) {
	region_desc_t r_gdt, r_idt;

	terminal_init();

	terminal_print("Initializing i386 architecture...\n");
	
	// Initialise the gdt
	SET_SEGMENT_LIMIT(gdt[GCODE_SEL], 0xFFFFF);
	SET_SEGMENT_LIMIT(gdt[GDATA_SEL], 0xFFFFF);
	SET_SEGMENT_LIMIT(gdt[GUCODE_SEL], 0xFFFFF);
	SET_SEGMENT_LIMIT(gdt[GUDATA_SEL], 0xFFFFF);
	SET_SEGMENT_LIMIT(gdt[GUFS_SEL], 0xFFFFF);
	SET_SEGMENT_LIMIT(gdt[GUGS_SEL], 0xFFFFF);

	// Set up quick access segment for per-cpu data
	pcpu_t* pc = &pcpus[0];
	SET_SEGMENT_LIMIT(gdt[GPRIV_SEL], sizeof(pcpu_t) - 1);
	SET_SEGMENT_BASE(gdt[GPRIV_SEL], (uintptr_t)pc);

	r_gdt.rd_limit = NGDT * sizeof(gdt[0]) - 1;
	r_gdt.rd_base = (uintptr_t)&gdt;
	lgdt(&r_gdt);

	kmalloc_init((char*)KMALLOC_START, KMALLOC_SIZE);

	if (is_errno(vm_phys_init(bootinfo->memory_map, bootinfo->memory_map_length)))
		PANIC("Physical memory initialization: FAILED");
	vm_phys_dump_info();

	kvm_space_init();

	load_bda();

	while (1)
		asm volatile("nop");
}