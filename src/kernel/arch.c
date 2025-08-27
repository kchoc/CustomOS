#include "kernel/memory/kmalloc.h"
#include "kernel/memory/page.h"
#include "kernel/memory/page_memory_alloc.h"
#include "kernel/memory/gdt.h"
#include "kernel/memory/layout.h"

#include "kernel/terminal.h"
#include "kernel/syscalls/syscalls.h"
#include "kernel/interrupts/idt.h"
#include "kernel/process/process.h"
#include "kernel/drivers/port_io.h"
#include "kernel/filesystem/fs.h"

void init() {
	terminal_init();
	printf("Terminal: OK\n");

	gdt_init();
	printf("GDT: OK\n");

	kmalloc_init((char*)KMALLOC_START, KMALLOC_SIZE);
	printf("kmalloc: OK\n");

	page_init();
	printf("Paging: OK\n");

	gdt_init();
	printf("GDT: OK\n");

	syscalls_init();
	printf("Syscalls: OK\n");

	idt_init();
	printf("IDT: READY\n");

	outb(0x21, 0xFD); // Mask all interrupts except IRQ1

	// Enable interrupts
	asm volatile("sti");
	printf("Interrupts: ENABLED\n");

	fat16_init();
	printf("FAT16: OK\n");

	tasking_init();
	printf("Tasking: OK\n");
}
