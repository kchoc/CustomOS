#ifndef PAGETABLE_H
#define PAGETABLE_H

#define PAGE_SIZE 4096
#define PAGE_BITS 12

#define PAGE_FLAG_USER      0
#define PAGE_FLAG_KERNEL    1
#define PAGE_FLAG_EXISTS    0
#define PAGE_FLAG_ALLOC     2
#define PAGE_FLAG_READONLY  0
#define PAGE_FLAG_READWRITE 4
#define PAGE_FLAG_NOCLEAR   0
#define PAGE_FLAG_CLEAR     8

struct page_entry {
	unsigned present:1;	// 1 = present
	unsigned readwrite:1;	// 1 = writable
	unsigned user:1;	// 1 = user mode
	unsigned writethrough:1;	// 1 = write through

	unsigned nocache:1;	// 1 = no caching
	unsigned accessed:1;	// 1 = accessed
	unsigned dirty:1;	// 1 = dirty
	unsigned pagesize:1;	// leave to zero

	unsigned globalpage:1;	// 1 if not to be flushed
	unsigned avail:3;

	unsigned addr:20;
};

struct page_table *page_table_create();
void page_table_init(struct page_table *table);
int page_table_map(struct page_table *table, void *virt, void *phys, int flags);
int page_table_getmap(struct page_table *table, void *virt, void **phys, int *flags);
void page_table_unmap(struct page_table *table, void *virt);

#endif