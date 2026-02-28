#ifndef MM_TYPES_H
#define MM_TYPES_H

#include <inttypes.h>
#include <stdbool.h>

#define PAGE_SIZE			4096
#define PAGE_MASK			(PAGE_SIZE - 1)
#define PAGE_ALIGN_DOWN(addr)	((addr) & ~PAGE_MASK)
#define PAGE_ALIGN_UP(addr)		(((addr) + PAGE_MASK) & ~PAGE_MASK)
#define VM_REGION_ALLOCATE_ADDR		((uintptr_t) -1)

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
	VM_PROT_ACCESSED    = 0x20,	// Accessed flag (for internal use)
	VM_PROT_DIRTY       = 0x40,	// Dirty flag (for internal use)
	VM_PROT_HUGE        = 0x80,	// Huge page flag (for internal use)
	VM_PROT_GLOBAL      = 0x100,// Global page flag (for internal use)
} vm_prot_t;

typedef enum vm_region_flags {
	VM_REG_F_NONE 		= 0x0,

	VM_REG_F_PRIVATE 	= 0x1,	// Default private (COW fork)
	VM_REG_F_SHARED 	= 0x2,	// Shared mapping (no COW, shared between processes)
	
	VM_REG_F_WIRED 		= 0x4,	// Never pageable
	VM_REG_F_DEVICE 	= 0x8,	// MMIO / physical device memory
	VM_REG_F_STACK 		= 0x10,	// Grows downwards
	VM_REG_F_FIXED 		= 0x20,	// Must use exact address
	VM_REG_F_KERNEL 	= 0x40,	// Map in kernel space (ignored for user regions)
	VM_REG_F_NOCACHE 	= 0x80,	// Don't allow caching of this region
} vm_region_flags_t;



typedef enum vm_obj_flags {
	VM_OBJ_F_KERNEL 	= 0x1,	// Map in kernel space
	VM_OBJ_F_NOSWAP 	= 0x2,	// Don't allow this region to be swapped out
	VM_OBJ_F_SHARED 	= 0x4,	// This region should be shared between processes (e.g. for shared memory)
	VM_OBJ_F_INTERNAL 	= 0x8,	// Used for VM internally
	VM_OBJ_F_SHADOWED 	= 0x10,	// This object is shadowed by another object (used for copy-on-write)
	VM_OBJ_F_WIRED 		= 0x20,	// Pages backing this object should be wired (i.e. not pageable)

} vm_obj_flags_t;

typedef enum vm_dev_flags {
	VM_DEV_READ  = 0x1,	// Device memory should be readable
	VM_DEV_WRITE = 0x2,	// Device memory should be writable
	VM_DEV_NOCACHE = 0x4, // Device memory should not be cached
} vm_dev_flags_t;

#endif // MM_TYPES_H
