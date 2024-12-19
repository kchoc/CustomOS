#include "kernel/memory/page.h"
#include "kernel/memory/layout.h"
#include "kernel/memory/pagetable.h"
#include "kernel/terminal.h"
#include "string.h"

static uint32_t pages_free = 0;
static uint32_t pages_total = 0;

static uint32_t *freemap = 0;
static uint32_t freemap_bits = 0;
static uint32_t freemap_bytes = 0;
static uint32_t freemap_cells = 0;
static uint32_t freemap_pages = 0;

static void *main_memory_start = (void *) MAIN_MEMORY_START;

#define CELL_BITS (8*sizeof(*freemap))

void page_init(void) {
    uint32_t total_memory = 512; // Example total memory in MB
    pages_total = (total_memory * 1024 * 1024 - MAIN_MEMORY_START) / PAGE_SIZE;
    pages_free = pages_total;
    printf("Total pages: %d\n", pages_total);

    freemap = (uint32_t *) main_memory_start;
    freemap_bits = pages_total;
    freemap_bytes = 1 + freemap_bits / 8;
    freemap_cells = 1 + freemap_bytes / CELL_BITS;
    freemap_pages = 1 + freemap_bytes / PAGE_SIZE;

    memset(freemap, 0xff, freemap_bytes);
    for (int i = 0; i < freemap_pages; i++) {
        page_alloc(0);
    }

    freemap[0] = 0x0;
}

void page_stats(uint32_t *total, uint32_t *free) {
    *total = pages_total;
    *free = pages_free;
}

void *page_alloc(uint8_t zero) {
    uint32_t cellmask;
    uint32_t pagenumber;
    void *pageaddr;

    if (!freemap) {
        printf("Error: freemap not initialized\n");
        return 0;
    }

    for (int i = 0; i < freemap_cells; i++) {
        if (freemap[i] != 0) {
            for (int j = 0; j < CELL_BITS; j++) {
                cellmask = 1 << j;
                if (freemap[i] & cellmask) {
                    freemap[i] &= ~cellmask;
                    pagenumber = i * CELL_BITS + j;
                    pageaddr = (pagenumber << PAGE_BITS) + main_memory_start;
                    if (zero) {
                        memset(pageaddr, 0, PAGE_SIZE);
                    }
                    pages_free--;
                    return pageaddr;
                }
            }
        }
    }

    printf("Error: out of memory\n");
    return 0;
}

void page_free(void *pageaddr) {
    uint32_t pagenumber = ((uintptr_t)pageaddr - (uintptr_t)main_memory_start) >> PAGE_BITS;
    uint32_t cellnumber = pagenumber / CELL_BITS;
    uint32_t celloffset = pagenumber % CELL_BITS;
    uint32_t cellmask = 1 << celloffset;
    freemap[cellnumber] |= cellmask;
    pages_free++;
}

void initialize_paging(void) {
    // Alocate memory for page directory and tables
    uint32_t *page_directory = page_alloc(1);
    uint32_t *first_page_table = page_alloc(1);

    // Map the first 4MB of memory to the first page table
    for (int i = 0; i < 1024; i++) {
        first_page_table[i] = (i * 0x1000) | 3; // Present, R/W
    }

    page_directory[0] = ((uint32_t)first_page_table) | 3; // Present, R/W

    for (int i = 1; i < 1024; i++) {
        page_directory[i] = 0;
    }

    asm volatile("mov %0, %%cr3" : : "r"(page_directory));

    uint32_t cr0;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000; // Enable paging
    asm volatile("mov %0, %%cr0" : : "r"(cr0));
}

void show_pages(void) {
    printf("Total pages: %d\n", pages_total);
    printf("Free  pages: %d\n", pages_free);
    printf("Inuse pages: %d\n", pages_total - pages_free);
}