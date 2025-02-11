#ifndef BITMAP_H
#define BITMAP_H

#include <stdint.h>

typedef struct {
    uint32_t *memory_map;
    uint32_t blocks;
} bitmap;

bitmap *create_bitmap(uint32_t blocks);
uint32_t allocate_blocks(bitmap *bm, uint32_t blocks);
void free_bitmap(bitmap *bm);
void free_blocks(bitmap *bm, uint32_t address, uint32_t blocks);

#endif // BITMAP_H