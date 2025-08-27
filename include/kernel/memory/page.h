#ifndef PAGE_H
#define PAGE_H

#include <stdint.h>

#define PAGE_SIZE 4096

#define PAGE_FLAG_PRESENT		0x1
#define PAGE_FLAG_READWRITE		0x2
#define PAGE_FLAG_USER			0x4
#define PAGE_FLAG_WRITETHROUGH	0x8
#define PAGE_FLAG_NOCACHE		0x10
#define PAGE_FLAG_ACCESSED		0x20
#define PAGE_FLAG_DIRTY			0x40
#define PAGE_FLAG_LARGE			0x80
#define PAGE_FLAG_GLOBAL		0x100

#define PAGE_FLAG_ALLOCATE      0x1000
#define PAGE_FLAG_CLEAR         0x2000

typedef uint32_t page_entry_t;

typedef struct page_table {
    page_entry_t entries[1024];
} page_table_t;

// Creates a page table
page_table_t *page_table_create(void);

// Gets the physical address and flags for a virtual address
int page_table_get_map(uint32_t virt, uint32_t *phys, uint32_t *flags);

// Maps a virtual address to a physical addresss
int page_table_map(uint32_t virt, uint32_t *phys, uint32_t  flags);

// Unmaps a virtual address
int page_table_unmap(uint32_t virt);

// Allocates a range of virtual addresses
void page_table_alloc(page_table_t *pd, uint32_t virt, uint32_t length, uint32_t flags);

// Frees a range of virtual addresses
void page_table_free(page_table_t *pd, uint32_t virt, uint32_t length);

// Destroys a page table and frees all associated pages
void page_table_destroy(page_table_t *pd);

// Duplicates a page table
page_table_t *new_task_page_table();

page_table_t *page_table_load(page_table_t *pd);
void page_table_enable();
void page_table_refresh();
page_table_t *get_current_page_table_phys();
page_table_t *get_current_page_table_virt();

#endif // PAGE_H
