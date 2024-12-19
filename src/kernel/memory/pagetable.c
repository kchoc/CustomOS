#include "kernel/memory/pagetable.h"
#include "kernel/memory/page.h"

#define ENTRIES_PER_TABLE (PAGE_SIZE/4)

struct page_table {
	struct page_entry entry[ENTRIES_PER_TABLE];
};

struct page_table *page_table_create() {
    return page_alloc(1);
}