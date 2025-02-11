#include "kernel/memory/kmalloc.h"
#include "kernel/memory/page.h"
#include "kernel/memory/gdt.h"
#include "kernel/memory/layout.h"

#include "kernel/terminal.h"
#include "kernel/interrupts/idt.h"
#include "kernel/task.h"
#include "kernel/drivers/port_io.h"

void init() {
	terminal_init();
	printf("Terminal: OK\n");

	gdt_init();
	printf("GDT: OK\n");

	if (initialize_paging())
		printf("Paging: OK\n");
	else
		printf("Paging: ERROR\n");
	
	kmalloc_init((char*)KMALLOC_START, KMALLOC_SIZE);
	printf("kmalloc: OK\n");

	gdt_init();
	printf("GDT: OK\n");

	idt_init();
	printf("IDT: READY\n");

	outb(0x21, 0xFD); // Mask all interrupts except IRQ1

	// Enable interrupts
	asm volatile("sti");
	printf("Interrupts: ENABLED\n");

	init_tasking();
	printf("Tasking: OK\n");
}
