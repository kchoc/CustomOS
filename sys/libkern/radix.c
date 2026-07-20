#include "radix.h"
#include <kern/errno.h>
#include <string.h>
#include <vm/kmalloc.h>

#define RADIX_NODE_SIZE(chunk_bits) (sizeof(radix_node_t) + sizeof(void*) * (1UL << (chunk_bits)))

/*
 * Allocate a node and zero it. This is required: every NULL check in
 * insert/lookup/remove assumes unused child slots are NULL, and kmalloc()
 * does not guarantee zeroed memory.
 */
static radix_node_t* radix_node_alloc(uint8_t chunk_bits)
{
    radix_node_t* node = kmalloc(RADIX_NODE_SIZE(chunk_bits));
    if (node)
        memset(node, 0, RADIX_NODE_SIZE(chunk_bits));
    return node;
}

void radix_tree_init(radix_tree_t* tree, uint8_t chunk_bits, uint8_t height)
{
    if (!tree || chunk_bits == 0 || height == 0)
        return;
    tree->chunk_bits = chunk_bits;
    tree->height     = height;
    tree->root       = NULL;
}

static void radix_node_destroy(radix_node_t* node, radix_tree_t* tree, uint8_t level,
                               void (*free_func)(void*))
{
    if (!node)
        return;
    if (level < tree->height - 1) {
        // Internal node: recursively destroy children
        for (size_t i = 0; i < (1UL << tree->chunk_bits); i++) {
            if (node->children[i])
                radix_node_destroy((radix_node_t*)node->children[i], tree, level + 1, free_func);
        }
    }
    else {
        // Leaf node: free values if free_func is provided
        if (free_func) {
            for (size_t i = 0; i < (1UL << tree->chunk_bits); i++) {
                if (node->children[i])
                    free_func(node->children[i]);
            }
        }
    }
    kfree(node);
}

void radix_tree_destroy(radix_tree_t* tree, void (*free_func)(void*))
{
    if (!tree)
        return;
    radix_node_destroy(tree->root, tree, 0, free_func);
    tree->root = NULL;
}

int radix_tree_insert(radix_tree_t* tree, unsigned long key, void* value)
{
    if (!tree || !value)
        return -EINVAL;

    if (!tree->root) {
        tree->root = radix_node_alloc(tree->chunk_bits);
        if (!tree->root)
            return -ENOMEM;
    }

    radix_node_t* node = tree->root;
    for (int level = 0; level < tree->height; level++) {
        size_t index = (key >> (level * tree->chunk_bits)) & ((1UL << tree->chunk_bits) - 1);

        if (level == tree->height - 1) {
            // Leaf level: insert value
            if (node->children[index])
                return -EEXIST; // caller must remove first if replacement is intended
            node->children[index] = value;
            node->count++;
            return 0;
        }

        radix_node_t* child = (radix_node_t*)node->children[index];
        if (!child) {
            child = radix_node_alloc(tree->chunk_bits);
            if (!child)
                return -ENOMEM;
            node->children[index] = child;
            node->count++;
        }
        node = child; // descend -- this was the missing step
    }
    return -1; // unreachable
}

void* radix_tree_lookup(radix_tree_t* tree, unsigned long key)
{
    if (!tree || !tree->root)
        return NULL;
    radix_node_t* node = tree->root;
    for (int level = 0; level < tree->height; level++) {
        size_t index = (key >> (level * tree->chunk_bits)) & ((1UL << tree->chunk_bits) - 1);
        node         = (radix_node_t*)node->children[index];
        if (!node)
            return NULL; // Not found
    }
    return node; // Found
}

static int radix_node_remove(radix_tree_t* tree, radix_node_t* node, int level, unsigned long key,
                             void** removed_value)
{
    size_t index = (key >> (level * tree->chunk_bits)) & ((1UL << tree->chunk_bits) - 1);

    if (level == tree->height - 1) {
        void* value = node->children[index];
        if (!value)
            return -ENOENT;
        node->children[index] = NULL;
        node->count--;
        if (removed_value)
            *removed_value = value;
        return 0;
    }

    radix_node_t* next = (radix_node_t*)node->children[index];
    if (!next)
        return -ENOENT;

    int res = radix_node_remove(tree, next, level + 1, key, removed_value);
    if (res)
        return res;

    if (next->count == 0) {
        kfree(next);
        node->children[index] = NULL;
        node->count--;
    }
    return 0;
}

int radix_tree_remove(radix_tree_t* tree, unsigned long key, void** removed_value)
{
    if (!tree || !tree->root)
        return -ENOENT;

    int res = radix_node_remove(tree, tree->root, 0, key, removed_value);
    if (res)
        return res;

    if (tree->root->count == 0) {
        kfree(tree->root);
        tree->root = NULL;
    }
    return 0;
}
