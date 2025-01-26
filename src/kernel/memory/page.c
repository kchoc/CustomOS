#include "kernel/memory/page.h"
#include "kernel/memory/pagetable.h"
#include "kernel/memory/layout.h"
#include "kernel/terminal.h"
#include "string.h"
#include "kernel/memory/physical_memory.h"

page_directory *current_page_directory = 0;

pt_entry *get_pt_entry(page_table *pt, virtual_address address) {
	if (pt) return &pt->entries[PT_INDEX(address)];
	return 0;
}

pd_entry *get_pd_entry(page_directory *pd, virtual_address address) {
	if (pd) return &pd->entries[PD_INDEX(address)];
	return 0;
}

pt_entry *get_page(virtual_address address) {
	page_directory *pd = current_page_directory;
	pd_entry *entry = &pd->entries[PD_INDEX(address)];
	page_table *table = (page_table *)PAGE_PHYS_ADDRESS(entry);

	pt_entry *page = &table->entries[PT_INDEX(address)];

	return page;
}

void *allocate_page(pt_entry *page) {
	void *block = allocate_blocks(1);
	if (block) {
		SET_FRAME(page, (physical_address)block);
		SET_ATTRIBUTE(page, PTE_PRESENT);
	}
	return block;
}

void *free_page(pt_entry *page) {
	void *address = (void *)PAGE_PHYS_ADDRESS(page);
	if (address)
		free_blocks(address, 1);

	CLEAR_ATTRIBUTE(page, PTE_PRESENT);
}

int set_page_directory(page_directory *pd) {
	if (!pd) return 0;

	current_page_directory = pd;

	asm volatile("movl %0, %%cr3" :: "r"(current_page_directory));

	return 1;
}

void check_page_directory(page_directory *pd, uint32_t table) {
	if (!pd) {
		printf("No page directory\n");
		for (int32_t i = 0; i < 1000000000; i++);
		return;
	}

	printf("Page Directory: %x\n", pd);

	for (uint32_t i = 1020; i < 1024; i++)
		printf("Directory Entry[%d]: %x\n", i, pd->entries[i]);

	page_table *table_ptr = (page_table *)PAGE_PHYS_ADDRESS(&pd->entries[table]);
	printf("Table %x\n", pd->entries[table]);
	printf("Table %x\n", table_ptr);

	for (uint32_t i = 1020; i < 1024; i++)
		printf("Table %d Entry[%d] %x\n", table, i, table_ptr->entries[i]);

	for (int i = 0; i < 1000000000; i++);
}

void flush_tlb_entry(virtual_address address) {
	asm volatile("cli; invlpg (%0); sti" :: "r"(address));
}

int map_page(void *phys_address, void *virt_address)
{
	// Get page directory
	page_directory *pd = current_page_directory;

	// Get page table
	pd_entry *entry = &pd->entries[PD_INDEX((uint32_t)virt_address)];

	// If page table is not present allocate it
	if ((*entry & PTE_PRESENT) != PTE_PRESENT) {
		page_table *table = allocate_blocks(1);
		if (!table) return 0; // ERROR: OUT OF MEMORY

		// Clear page table
		memset(table, 0, sizeof(page_table));

		// Create new entry
		pd_entry *entry = &pd->entries[PD_INDEX((uint32_t)virt_address)];

		// Map in the table and set attributes
		SET_ATTRIBUTE(entry, PDE_PRESENT);
		SET_ATTRIBUTE(entry, PDE_READ_WRITE);
		SET_FRAME(entry, (physical_address)table);
	}

	// Get table
	page_table *table = (page_table *)PAGE_PHYS_ADDRESS(entry);

	// Get page in table
	pt_entry *page = &table->entries[PT_INDEX((uint32_t)virt_address)];

	// Map in page

	SET_ATTRIBUTE(page, PTE_PRESENT);
	SET_FRAME(page, (uint32_t)phys_address);
}

// Unmap a page
void unmap_page(void *virt_address) {
	pt_entry *page = get_page((uint32_t)virt_address);

	SET_FRAME(page, 0); // Set physical address to 0 (effectively this is now a null pointer)
	CLEAR_ATTRIBUTE(page, PTE_PRESENT); // Set as not present
}

// Initialize virtual memory manager
int initialize_paging(void) {
	// Create a default page directory
	page_directory *pd = (page_directory *)allocate_blocks(1);
	if (!pd) return 0; // ERROR: OUT OF MEMORY

	// Clear page directory and set as current
	memset(pd, 0, sizeof(page_directory));
	for (uint32_t i = 0; i < 1024; i++)
		pd->entries[i] = 0;

	// Allocate a default page table
	page_table *table = (page_table *)allocate_blocks(1);
	if (!table) return 0; // ERROR: OUT OF MEMORY

	// Allocate a 3GB page table
	page_table *table3G = (page_table *)allocate_blocks(1);
	if (!table3G) return 0; // ERROR: OUT OF MEMORY

	// Clear page tables
	memset(table,   0, sizeof(page_table));
	memset(table3G, 0, sizeof(page_table));

	// Identity map first 4MB of memory for kernel
	for (uint32_t i = 0, frame = 0x0, virt = 0x0; i < 1024; i++, frame += PAGE_SIZE, virt+= PAGE_SIZE) {
		// Create new page
		pt_entry page = 0;
		SET_ATTRIBUTE(&page, PTE_PRESENT);
		SET_ATTRIBUTE(&page, PTE_READ_WRITE);
		SET_FRAME(&page, frame);

		// Add page to 3GB page table
		table3G->entries[PT_INDEX(virt)] = page;
	}

	// Map Kernel to 3GB+ addresses (higher half kernel)
	for (uint32_t i = 0, frame = KERNEL_ADDRESS, virt = 0xC0000000; i < 1024; i++, frame += PAGE_SIZE, virt += PAGE_SIZE) {
		pt_entry page = 0;
		SET_ATTRIBUTE(&page, PTE_PRESENT);
		SET_ATTRIBUTE(&page, PTE_READ_WRITE);
		SET_FRAME(&page, frame);

		// Add page to 3GB+ page table
		table->entries[PT_INDEX(virt)] = page;
	}

	pd_entry *entry = &pd->entries[PD_INDEX(0xC0000000)];
	SET_ATTRIBUTE(entry, PDE_PRESENT);
	SET_ATTRIBUTE(entry, PDE_READ_WRITE);
	SET_FRAME(entry, (physical_address)table); // 3GB directory entry points to default page table

	pd_entry *entry2 = &pd->entries[PD_INDEX(0x00000000)];
	SET_ATTRIBUTE(entry2, PDE_PRESENT);
	SET_ATTRIBUTE(entry2, PDE_READ_WRITE);
	SET_FRAME(entry2, (physical_address)table3G); // Default directory entry points to 3GB page table

	// Switch to page directory
	set_page_directory(pd);

	// Enable paging: Set Paging Bit
	asm volatile("movl %CR0, %EAX; orl $0x80000001, %EAX; movl %EAX, %CR0");

	return 1;
}
