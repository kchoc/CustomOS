#include "kernel/memory/physical_memory.h"
#include "kernel/memory/layout.h"
#include "kernel/terminal.h"
#include "string.h"

uint32_t memory_map[BLOCKS / 32];

void *allocate_blocks(uint32_t blocks) {
    uint32_t i_position = 0;
    uint8_t j_position = 0;
    uint32_t count = 0;

    // Iterate through memory map
    for (uint32_t i = 0; i < BLOCKS/32; i++) {

        // Skip if all blocks are allocated
        if (memory_map[i] == 0xFFFFFFFF)
            continue;

        // Iterate through bits
        for (uint8_t j = 0; j < 32; j++) {
            // If block is allocated reset count and position to next block
            if (memory_map[i] & (1 << j)) {
                count = 0;
                i_position = i;
                j_position = j + 1;
                continue;
            }

            // Increment count and check if we have enough blocks
            count++;
            if (count == blocks) {
                void *position = (void *)BLOCK_START_ADDRESS + (i_position * 32 + j_position) * PAGE_SIZE;
                if (j_position + count >= 32) {
                    memory_map[i_position] |= ~(1 << j_position - 1);
                    count -= 32 - j_position;
                    j_position = 0;
                    i_position++;
                } else {
                    memory_map[i_position] |= (1 << (j_position + count)) - (1 << j_position);
                    return position;
                }

                while (count >= 32) {
                    memory_map[i_position] = 0xFFFFFFFF;
                    i_position++;
                    count -= 32;
                }
                memory_map[i_position] |= (1 << (count + 1)) - 1;

                return position;
            }
        }
    }
    return 0;
}

void free_blocks(void *address, uint32_t blocks) {
    uint32_t position = ((uint32_t)address - BLOCK_START_ADDRESS) / PAGE_SIZE;
    for (uint32_t i = 0; i < blocks; i++) {
        memory_map[position + i / 32] &= ~(1 << (i % 32));
    }
    memset(address, 0, blocks * PAGE_SIZE);
}
