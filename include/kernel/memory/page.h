#ifndef PAGE_H
#define PAGE_H

#include <stdint.h>

void page_init(void);
void *page_alloc(uint8_t zero);
void page_free(void *page);
void page_stats(uint32_t *total, uint32_t *free);
void initialize_paging(void);
void show_pages(void);

#endif // PAGE_H