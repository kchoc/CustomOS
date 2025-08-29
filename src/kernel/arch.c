#include "kernel/memory/kmalloc.h"
#include "kernel/memory/vm.h"
#include "kernel/memory/mm.h"
#include "kernel/memory/vmspace.h"
#include "kernel/memory/layout.h"

#include "kernel/descriptors/gdt.h"
#include "kernel/descriptors/idt.h"
#include "kernel/descriptors/rsd.h"

#include "kernel/terminal.h"
#include "kernel/syscalls/syscalls.h"
#include "kernel/process/process.h"
#include "kernel/drivers/port_io.h"
#include "kernel/filesystem/fs.h"

void init() {
	terminal_init();
	printf("Terminal: OK\n");

	kmalloc_init((char*)KMALLOC_START, KMALLOC_SIZE);
	printf("kmalloc: OK\n");

	pmm_init();
	printf("PMM: OK\n");

	vmm_init();
	printf("VMM: OK\n");

	vm_space_init();
	printf("VMSpace: OK\n");

	gdt_init();
	printf("GDT: OK\n");

	// rsdt_init();
	// printf("RSD: OK\n");

	syscalls_init();
	printf("Syscalls: OK\n");

	idt_init();
	printf("IDT: OK\n");
	
	outb(0x21, 0xFD); // Mask all interrupts except IRQ1

	// Enable interrupts
	asm volatile("sti");
	printf("Interrupts: ENABLED\n");

	fat16_init();
	printf("FAT16: OK\n");

	tasking_init();
	printf("Tasking: OK\n");
	delay(100);

}
