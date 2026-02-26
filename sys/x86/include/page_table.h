#ifndef PAGE_TABLE_H
#define PAGE_TABLE_H

#include <inttypes.h>

#define PAGE_SIZE 4096
#define HUGE_PAGE_SIZE (PAGE_SIZE * 1024)
#define PAE_HUGE_PAGE_SIZE (PAGE_SIZE * 512)
#define LONG_MODE_PAGE_SIZE PAGE_SIZE
#define LONG_MODE_HUGE_PAGE_SIZE (PAGE_SIZE * 512)
#define LONG_MODE_GIGA_PAGE_SIZE (PAGE_SIZE * 1024 * 1024 * 256)

typedef uintptr_t page_entry_t;

typedef struct page_table_t {
	page_entry_t entries[1024];
} page_table_t __attribute__((aligned(4096)));

#endif // PAGE_TABLE_H
