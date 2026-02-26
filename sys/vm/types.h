#ifndef MM_TYPES_H
#define MM_TYPES_H

#include <inttypes.h>
#include <stdbool.h>

#define PAGE_SIZE			4096
#define PAGE_MASK			(PAGE_SIZE - 1)
#define PAGE_ALIGN(addr)	(((addr) + PAGE_MASK) & ~PAGE_MASK)

typedef uintptr_t vaddr_t;
typedef uintptr_t paddr_t;

typedef uint64_t vm_ooffset_t; // Object Offset

typedef enum vm_prot {
	VM_PROT_NONE        = 0x0,
	VM_PROT_READ        = 0x1,
	VM_PROT_WRITE       = 0x2,
	VM_PROT_USER 	 	= 0x4,
	VM_PROT_PWT	   		= 0x8,	// Page-level write-through
	VM_PROT_NOCACHE   	= 0x10,	// Page-level cache disable
	VM_PROT_ACCESSED    	= 0x20,	// Accessed flag (for internal use)
	VM_PROT_DIRTY       	= 0x40,	// Dirty flag (for internal use)
	VM_PROT_HUGE        	= 0x80,	// Huge page flag (for internal use)
	VM_PROT_GLOBAL      	= 0x100,// Global page flag (for internal use)
} vm_prot_t;

typedef enum vm_flags {
	VM_MAP_FIXED        = 0x1,	// Interpret 'virt' as fixed address
	VM_MAP_ANON        	= 0x2,	// Map anonymous memory (not backed by any file)
	VM_MAP_SHARED 		= 0x4,	// Shared mapping
	VM_MAP_PRIVATE      = 0x8,	// Private mapping (copy-on-write)
	VM_MAP_GLOWSDOWN    = 0x10,	// Allocate from low addresses
	VM_MAP_FORCE       	= 0x20,	// Force mapping even if region is occupied
	VM_MAP_FREE		 	= 0x40,	// Free existing mappings in the region before mapping
	VM_MAP_PHYS      	= 0x80,	// Map to specific physical address
	VM_MAP_ZERO      	= 0x100,// Zero out the mapped memory

} vm_flags_t;

#endif // MM_TYPES_H
