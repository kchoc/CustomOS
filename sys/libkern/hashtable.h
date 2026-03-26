#ifndef HASHTABLE_H
#define HASHTABLE_H

#include <list.h>
#include <stddef.h>
#include <stdint.h>

typedef struct hashtable_entry {
    uint64_t    key;
    list_node_t node; // Node for chaining entries in the same bucket
} hashtable_entry_t;

typedef struct hashtable {
    list_t* buckets;
    size_t  bucket_count;
} hashtable_t;

int hashtable_create(size_t bucket_count, hashtable_t** result);
int hashtable_destroy(hashtable_t* ht, void (*free_entry)(void*));
int hashtable_get(hashtable_t* ht, uint64_t key, hashtable_entry_t** result);
int hashtable_put(hashtable_t* ht, void* entry);
int hashtable_remove(hashtable_t* ht, uint64_t key, void (*free_entry)(void*));

#endif // HASHTABLE_H
