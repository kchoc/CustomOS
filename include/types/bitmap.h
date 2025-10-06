#ifndef BITMAP_H
#define BITMAP_H

#include "kernel/types.h"

typedef struct bitmap {
    uint32_t *memory_map;
    uint32_t total_blocks;
    uint32_t free_blocks;
} bitmap_t;

bitmap_t *create_bitmap(uint32_t blocks);

int allocate_block(bitmap_t *bm);
int allocate_blocks(bitmap_t *bm, uint32_t blocks);

void free_bitmap(bitmap_t *bm);

void set_block(bitmap_t *bm, uint32_t address, uint8_t state);
void set_blocks(bitmap_t *bm, uint32_t address, uint32_t blocks, uint8_t state);

#endif // BITMAP_H