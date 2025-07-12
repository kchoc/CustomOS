#ifndef BITMAP_H
#define BITMAP_H

#include <stdint.h>

typedef struct bitmap {
    uint32_t *memory_map;
    uint32_t total_blocks;
    uint32_t free_blocks;
} bitmap_t;

bitmap_t *create_bitmap(uint32_t blocks);

uint32_t allocate_block(bitmap_t *bm);
uint32_t allocate_blocks(bitmap_t *bm, uint32_t blocks);

void free_bitmap(bitmap_t *bm);

void free_block(bitmap_t *bm, uint32_t address);
void free_blocks(bitmap_t *bm, uint32_t address, uint32_t blocks);

#endif // BITMAP_H