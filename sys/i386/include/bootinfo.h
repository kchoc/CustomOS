#ifndef X86_I386_BOOTINFO_H
#define X86_I386_BOOTINFO_H

#include <inttypes.h>
#include <vm/vm_phys.h>

typedef struct bootinfo_t {
	memory_map_entry_t* memory_map;
	size_t memory_map_length;
} bootinfo_t;

extern bootinfo_t* bootinfo;

#endif // X86_I386_BOOTINFO_H
