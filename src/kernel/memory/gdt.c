#include "kernel/memory/gdt.h"

#define GDT_ENTRIES 5
struct gdt_entry gdt[GDT_ENTRIES];
struct gdt_ptr gp;

// Initialize GDT
void gdt_init() {

	gdt[0] = (struct gdt_entry){0}; // Null descriptor

	// User Code Segment (0x00000000 - 0x3FFFFFFF)
	gdt[1] = (struct gdt_entry) {
		.limit_low = 0xFFFF,
		.base_low = 0x0000,
		.base_middle = 0x00,
		.access = 0x9A, // Present, Ring 0, Code, Read/Execute
		.granularity = 0xCF, // 4KB, 32-bit
		.base_high = 0x00
	};

	// User Data Segment (0x00000000 - 0x3FFFFFFF)
	gdt[2] = (struct gdt_entry) {
		.limit_low = 0xFFFF,
		.base_low = 0,
		.base_middle = 0,
		.access = 0x92, // Present, Ring 0, Data, Read/Write
		.granularity = 0xCF, // 4KB, 32-bit
		.base_high = 0x00
	};

	// Kernel Code Segment (0xC0000000 - 0xFFFFFFFF)
	gdt[3] = (struct gdt_entry) {
		.limit_low = 0xFFFF,
		.base_low = 0,
		.base_middle = 0,
		.access = 0x9A, // Present, Ring 0, Code, Read/Execute
		.granularity = 0xCF, // 4KB, 32-bit
		.base_high = 0xC0
	};

	// Kernel Data Segment (0xC0000000 - 0xFFFFFFFF)
	gdt[4] = (struct gdt_entry) {
		.limit_low = 0xFFFF,
		.base_low = 0,
		.base_middle = 0,
		.access = 0x92, // Present, Ring 0, Data, Read/Write
		.granularity = 0xCF, // 4KB, 32-bit
		.base_high = 0xC0
	};

	gp.size = sizeof(gdt) - 1;
	gp.offset = (uint32_t)&gdt;

	// Load GDT
	load_gdt(&gp);

	// Load segment registers after setting up the GDT
	asm volatile (
		"mov $0x10, %%ax\n"
		"mov %%ax, %%ds\n"
		"mov %%ax, %%es\n"
		"mov %%ax, %%fs\n"
		"mov %%ax, %%gs\n"
		"mov %%ax, %%ss\n"
		:
		:
		: "ax"
	);
}
