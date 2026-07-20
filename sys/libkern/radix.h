#ifndef RADIX_H
#define RADIX_H
#include <inttypes.h>
#include <limits.h>
#include <stddef.h>

/*
 * Sizing for vm_object page trees (and any other user of this radix tree
 * that indexes on a native `unsigned long` key).
 *
 * VM_RADIX_CHUNK_BITS = 9 gives 512 children per node. On a 64-bit-pointer
 * build that makes RADIX_NODE_SIZE(9) exactly one page (4096 bytes), which
 * is a convenient allocation unit.
 *
 * VM_RADIX_HEIGHT is derived from sizeof(unsigned long) so it is correct on
 * both i386 (32-bit long -> height 4, covers 36 bits) and amd64 (64-bit
 * long -> height 8, covers 72 bits) without any #ifdef. It always covers at
 * least BITS_PER_LONG bits, and (height - 1) * chunk_bits is always less
 * than BITS_PER_LONG, so the key shift in insert/lookup/remove is never UB.
 */
#define BITS_PER_LONG       (sizeof(unsigned long) * CHAR_BIT)
#define VM_RADIX_CHUNK_BITS 9
#define VM_RADIX_HEIGHT     ((BITS_PER_LONG + VM_RADIX_CHUNK_BITS - 1) / VM_RADIX_CHUNK_BITS)

typedef struct radix_tree {
    uint8_t            chunk_bits; // log2(children per node), i.e. 1 -> 2^chunk_bits children
    uint8_t            height;     // Height of the tree (number of levels)
    struct radix_node* root;       // Pointer to the root node of the tree
} radix_tree_t;

typedef struct radix_node {
    size_t count;      // Number of non-NULL direct children (internal) or entries (leaf)
    void*  children[]; // Flexible array of pointers to child nodes (internal) or data (leaf)
} radix_node_t;

void  radix_tree_init(radix_tree_t* tree, uint8_t chunk_bits, uint8_t height);
void  radix_tree_destroy(radix_tree_t* tree, void (*free_func)(void*));
int   radix_tree_insert(radix_tree_t* tree, unsigned long key, void* value);
void* radix_tree_lookup(radix_tree_t* tree, unsigned long key);
int   radix_tree_remove(radix_tree_t* tree, unsigned long key, void** removed_value);

#endif // RADIX_H
