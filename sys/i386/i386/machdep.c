#include "machdep.h"
#include "idt.h"
#include "gdt.h"
#include "rsd.h"

#include <dev/pci/pci.h>

#include <sys/pcpu.h>

#include <fs/vfs.h>
#include <fs/sockfs.h>
#include <fs/fat16.h>

#include <vm/kmalloc.h>
#include <vm/vm_phys.h>
#include <vm/vm_space.h>
#include <vm/vm_map.h>
#include <vm/layout.h>

#include <i386/bios/bda.h>

#include <machine/bootinfo.h>
#include <machine/segment_i386.h>

#include <kern/syscalls.h>
#include <kern/process.h>
#include <kern/elf.h>
#include <kern/terminal.h>
#include <kern/panic.h>
#include <kern/pit.h>
#include <kern/errno.h>
#include <kern/lapic.h>
#include <kern/system_init.h>

void init386(void) {
	region_desc_t r_gdt;

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
  write_tss(&pc->tss, 0x10, KERNEL_STACK_TOP); // Set kernel stack segment and pointer in TSS
	SET_SEGMENT_LIMIT(gdt[GPRIV_SEL], 0xFFFFF);
	SET_SEGMENT_BASE(gdt[GPRIV_SEL], (uintptr_t)pc);
  SET_SEGMENT_BASE(gdt[GPROC0_SEL], (uintptr_t)&pc->tss);

	r_gdt.rd_limit = NGDT * sizeof(gdt[0]) - 1;
	r_gdt.rd_base = (uintptr_t)&gdt;
	lgdt(&r_gdt);
  load_tss(GPROC0_SEL << 3);

	kmalloc_init((char*)KMALLOC_START, KMALLOC_SIZE);

	if (is_errno(vm_phys_init(bootinfo->memory_map, bootinfo->memory_map_length)))
		PANIC("Physical memory initialization: FAILED");
	vm_phys_dump_info();

	kvm_space_init();
	printf("Kernel VM space initialized.\n");

	if (is_errno(load_bda()))
		PANIC("BDA initialization: FAILED");

	if (is_errno(rsdt_init()))
		PANIC("RSDT initialization: FAILED");

	syscalls_init();	

	idt_init();	

  lapic_init();
  pcpu_init(0);

	tasking_init();

  asm volatile("sti"); // Enable interrupts
  printf("Interrupts enabled.\n");

	printf("PCI Enumeration:\n");
	pci_bus.enumerate(&pci_bus);

	vfs_list_block_devices();

  // Initialize and mount root filesystem
  printf("VFS: %s\n", vfs_mount_drive("hdap1", "/", &fat16_fs_type) == 0 ? "OK" : "FAILED");

  // Initialize sockfs
  printf("Sockfs: %s\n", sockfs_init() == 0 ? "OK" : "FAILED");

  system_init();

  // list_tasks();

	while (1)
		asm volatile("nop");
}
