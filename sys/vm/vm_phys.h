#ifndef VM_PHYS_H
#define VM_PHYS_H

#include <vm/types.h>

#include <inttypes.h>
#include <stddef.h>

#define MEM_MAP_TYPE_AVAILABLE			1
#define MEM_MAP_TYPE_RESERVED			2
#define MEM_MAP_TYPE_ACPI_RECLAIMABLE	3
#define MEM_MAP_TYPE_ACPI_NVS			4
#define MEM_MAP_TYPE_BADRAM				5

typedef struct memory_map_entry_t {
	uint64_t base_addr;
	uint64_t length;
	uint32_t type;
	uint32_t acpi_ext_attr;
} memory_map_entry_t;

int 	vm_phys_init(memory_map_entry_t* mem_map, size_t mem_map_length);
void 	vm_phys_dump_info();
paddr_t vm_phys_alloc_page();
paddr_t vm_phys_alloc_pages(size_t npages);
void 	vm_phys_alloc_specific_page(paddr_t phys);
void 	vm_phys_alloc_specific_pages(paddr_t phys, size_t npages);
void 	vm_phys_free_page(paddr_t phys);
void 	vm_phys_free_pages(paddr_t phys, size_t npages);

#endif // VM_PHYS_H
