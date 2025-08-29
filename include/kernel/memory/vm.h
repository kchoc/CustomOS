#ifndef VM_H
#define VM_H

#include <stdint.h>
#include <stddef.h>

#define PAGE_SIZE 4096

#define VM_PROT_PRESENT         0x1
#define VM_PROT_READWRITE       0x2
#define VM_PROT_USER            0x4
#define VM_PROT_WRITETHROUGH    0x8
#define VM_PROT_NOCACHE         0x10
#define VM_PROT_ACCESSED        0x20
#define VM_PROT_DIRTY			0x40
#define VM_PROT_LARGE			0x80
#define VM_PROT_GLOBAL          0x100

#define VM_MAP_ZERO             0x1 // Zero the pages
#define VM_MAP_PHYS             0x2 // Mapping is to a physical address

typedef uint32_t page_entry_t;

typedef struct page_table {
    page_entry_t entries[1024];
} page_table_t __attribute__((aligned(PAGE_SIZE)));

extern page_entry_t kernel_pd_entries[255];

void    vmm_init(void);
void   *vmm_map(void *virt, uintptr_t phys, size_t size, int prot, int flags);
void    vmm_unmap(void *virt, size_t size);
uint32_t vmm_resolve(void *virt);
void    vmm_zero(uintptr_t phys);

void *io_remap(uintptr_t phys, size_t size);
void io_unmap(void *virt, size_t size);

page_table_t* get_current_page_directory_phys();

#endif // VM_H
