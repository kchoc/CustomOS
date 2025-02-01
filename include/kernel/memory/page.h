#ifndef PAGE_H
#define PAGE_H

#include <stdint.h>

// Constants
#define PAGE_DIRECTORY_SIZE 1024
#define PAGE_TABLE_SIZE 1024
#define PAGE_SIZE 4096

// Macros

#define PD_INDEX(virt_address) ((virt_address) >> 22)
#define PT_INDEX(virt_address) ((virt_address) >> 12) & 0x3FF

#define PAGE_PHYS_ADDRESS(dir_entry) ((*dir_entry) & 0xFFFFF000)

#define SET_ATTRIBUTE(entry, attr) (*entry |= attr)
#define CLEAR_ATTRIBUTE(entry, attr) (*entry &= ~attr)
#define TEST_ATTRIBUTE(entry, attr) (*entry & attr)
#define SET_FRAME(entry, address) (*entry = (*entry & ~0xFFFFF000) | address)

// Typedefs

typedef uint32_t pt_entry;
typedef uint32_t pd_entry;
typedef uint32_t virtual_address;
typedef uint32_t physical_address;

typedef enum {
	PTE_PRESENT			= 0x01,
	PTE_READ_WRITE		= 0x02,
	PTE_USER	  		= 0x04,
	PTE_WRITE_THROUGH	= 0x08,
	PTE_CACHE_DISABLE	= 0x10,
	PTE_ACCESSED		= 0x20,
	PTE_DIRTY			= 0x40,
	PTE_PAT				= 0x80,
	PTE_GLOBAL			= 0x100,
	PTE_FRAME			= 0x7FFFF000	// Bits 12+
} PAGE_TABLE_FLAGS;

typedef enum {
	PDE_PRESENT			= 0x01,
	PDE_READ_WRITE		= 0x02,
	PDE_USER			= 0x04,
	PDE_WRITE_THROUGH	= 0x08,
	PDE_CACHE_DISABLE	= 0x10,
	PDE_ACCESSED		= 0x20,
	PDE_DIRTY			= 0x40,		// 4MB entries only
	PDE_PAGE_SIZE		= 0x80,		// 0 = 4KB page, 1 = 4MB page
	PDE_GLOBAL			= 0x100,	// 4MB entries only
	PDE_PAT				= 0x200,	// 4MB entries only
	PDE_FRAME			= 0x7FFFF000	// Bits 12+
} PAGE_DIRECTORY_FLAGS;

// Page Table: 4MB, 1024 entries
typedef struct {
	pt_entry entries[PAGE_TABLE_SIZE];
} page_table;

// Page Directory: 4GB, 1024 tables
typedef struct {
	pd_entry entries[PAGE_TABLE_SIZE];
} page_directory;

static page_directory *current_page_directory = 0;

// Functions

pt_entry *get_pt_entry(page_table* pt,	   virtual_address address);
pd_entry *get_pd_entry(page_directory* pd, virtual_address address);
pt_entry *get_page(const virtual_address address);

void *allocate_page(pt_entry *page);
void *free_page(pt_entry *page);

int set_page_directory(page_directory *pd);
void flush_tlb_entry(virtual_address address);

void check_page_directory(page_directory *pd, uint32_t table);

int map_page(void *phys_address, void *virt_address);
void unmap_page(void *virt_address);

int initialize_paging(void);

#endif // PAGE_H
