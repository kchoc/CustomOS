#include "kernel/memory/page.h"
#include "kernel/memory/layout.h"
#include "kernel/terminal.h"
#include "string.h"
#include "kernel/memory/physical_memory.h"

page_directory initial_pd __attribute__((aligned(4096)));
page_table kernel_tables[256] __attribute__((aligned(4096)));

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

	asm volatile("movl %0, %%cr3" :: "r"((physical_address)current_page_directory & 0x3FFFF000));
	
	return 1;
}

void check_page_directory(page_directory *pd, uint32_t table) {
	if (!pd) {
		printf("No page directory\n");
		return;
	}

	printf("Page Directory: %x\n", pd);
	page_table *table_ptr = (page_table *)(&pd->entries[table] + 0xC0000000);
	for (uint32_t i = 0; i < 4; i++)
		printf("Directory Entry[%d]:    %x    Table %d Entry[%d]:    %x\n", i, pd->entries[i], table, i, table_ptr->entries[i]);
	printf("...\n");
	for (uint32_t i = 1020; i < 1024; i++)
		printf("Directory Entry[%d]: %x    Table %d Entry[%d]: %x\n", i, pd->entries[i], table, i, table_ptr->entries[i]);
}

void flush_tlb_entry(virtual_address address) {
	asm volatile(
		"cli\n"
		"invlpg (%0)\n"
		"sti" :: "r"(address));
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
	// Clear page directory and set as current
	memset(&initial_pd, 0, sizeof(page_directory));
	for (uint32_t i = 0; i < 1024; i++)
		initial_pd.entries[i] = 0;
	
	memset(&kernel_tables, 0, sizeof(kernel_tables));

	uint32_t frame = 0x00000000;
	for (uint32_t i = 0; i < 256; i++) {
		page_table *table = &kernel_tables[i];

		for (uint32_t j = 0; j < 1024; j++) {

			pt_entry page = PTE_PRESENT | PTE_READ_WRITE;
			SET_FRAME(&page, frame);

			table->entries[j] = page;
			frame += PAGE_SIZE;
		}

		initial_pd.entries[i + 768] = ((physical_address)table & 0x3FFFF000) | PDE_PRESENT | PDE_READ_WRITE;
	}

	// Map VGA memory
	kernel_tables[0].entries[1023] = 0x000B8000 | PTE_PRESENT | PTE_READ_WRITE;


	// check_page_directory(&initial_pd, 768);

	// Switch to page directory
	set_page_directory(&initial_pd);

	return 1;
}
