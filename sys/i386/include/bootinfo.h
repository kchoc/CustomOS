#ifndef _I386_BOOTINFO_H_
#define _I386_BOOTINFO_H_

#include <vm/vm_phys.h>

#include <inttypes.h>

typedef struct bootinfo_t {
	memory_map_entry_t* memory_map;
	size_t memory_map_length;
} bootinfo_t;

extern bootinfo_t* bootinfo;

#endif // _I386_BOOTINFO_H_
