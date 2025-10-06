#include "types/bitmap.h"
#include "kernel/memory/kmalloc.h"

bitmap_t *create_bitmap(uint32_t blocks) {
    bitmap_t *bm = (bitmap_t *)kmalloc(sizeof(bitmap_t));
    bm->total_blocks = blocks;
    bm->free_blocks = blocks;
    bm->memory_map = (uint32_t *)kmalloc(blocks >> 3);
    return bm;
}

/* Allocate a single block and return its index, or -1 if none are available */
int allocate_block(bitmap_t *bm) {
    for (uint32_t i = 0; i < bm->total_blocks >> 5; i++) {
        if (bm->memory_map[i] == 0xFFFFFFFF)
            continue;

        for (uint8_t j = 0; j < 32; j++) {
            if (!(bm->memory_map[i] & (1 << j))) {
                bm->memory_map[i] |= 1 << j;
                bm->free_blocks--;
                return (i << 5) | j;
            }
        }
    }
    return -1;
}

/* Allocate multiple contiguous blocks and return the starting index, or -1 if none are available */
int allocate_blocks(bitmap_t *bm, uint32_t blocks) {
    uint32_t i_position = 0;
    uint8_t j_position = 0;
    uint32_t count = 0;
    
    bm->free_blocks -= blocks;
    for (uint32_t i = 0; i < bm->total_blocks >> 5; i++) {
        if (bm->memory_map[i] == 0xFFFFFFFF)
            continue;

        for (uint8_t j = 0; j < 32; j++) {
            // If block is allocated reset count and position to next block
            if (bm->memory_map[i] & (1 << j)) {
                count = 0;
                i_position = i;
                j_position = j + 1;
                continue;
            }

            count++;
            // If we have enough blocks return position and update memory map
            if (count == blocks) {
                uint32_t position = (i_position << 5) | j_position;
                // If we need to move to the next section
                if (j_position + count >= 32) {
                    bm->memory_map[i_position] |= ~((1 << j_position) - 1);
                    count -= 32 - j_position;
                    j_position = 0;
                    i_position++;
                } else {
                    bm->memory_map[i_position] |= (1 << (j_position + count)) - (1 << j_position);
                    return position;
                }

                // Set all bits in the section to 1
                while (count >= 32) {
                    bm->memory_map[i_position] = 0xFFFFFFFF;
                    i_position++;
                    count -= 32;
                }
                // Set remaining bits
                bm->memory_map[i_position] |= (1 << (count + 1)) - 1;

                return position;
            }
        }
    }
    bm->free_blocks += blocks;
    return -1;
}

void free_bitmap(bitmap_t *bm) {
    kfree(bm->memory_map);
    kfree(bm);
}

void set_block(bitmap_t *bm, uint32_t address, uint8_t state) {
    uint32_t section = address >> 5;
    uint32_t offset = address & 0x1F;
    bm->free_blocks += state ? -1 : 1;
    if (state)
        bm->memory_map[section] |= 1 << offset;
    else
        bm->memory_map[section] &= ~(1 << offset);
}

void set_blocks(bitmap_t* bm, uint32_t address, uint32_t blocks, uint8_t state) {
    uint32_t section = address >> 5;
    uint32_t offset = address & 0x1F;
    bm->free_blocks += state ? -blocks : blocks;
    
    if (offset + blocks < 32) {
        if (state)
            bm->memory_map[section] |= ((1 << blocks) - 1) << offset;
        else
            bm->memory_map[section] &= ~(((1 << blocks) - 1) << offset);
        return;
    }
    if (state)
        bm->memory_map[section] |= 0xFFFFFFFF << offset;
    else
        bm->memory_map[section] &= (1 << offset) - 1;
    blocks -= 32 - offset;
    section++;
    while (blocks >= 32) {
        bm->memory_map[section] = state ? 0xFFFFFFFF : 0;
        section++;
        blocks -= 32;
    }
    if (state)
        bm->memory_map[section] |= (1 << blocks) - 1;
    else
        bm->memory_map[section] &= ~((1 << blocks) - 1);
}
