#include "hashtable.h"

#include <vm/kmalloc.h>

#include <kern/errno.h>

int hashtable_create(size_t bucket_count, hashtable_t** result) {
    hashtable_t* ht = kmalloc(sizeof(hashtable_t));
    if (!ht) return -ENOMEM;

    ht->bucket_count = bucket_count;
    ht->buckets = kmalloc(bucket_count * sizeof(list_t));
    if (!ht->buckets) {
        kfree(ht);
        return -ENOMEM;
    }

    for (size_t i = 0; i < bucket_count; i++) {
        list_init(&ht->buckets[i], 0);
    }

    *result = ht;
    return 0; // Success
}

int hashtable_destroy(hashtable_t* ht, void (*free_entry)(void*)) {
    if (!ht) return -EINVAL;

    for (size_t i = 0; i < ht->bucket_count; i++) {
        list_node_t* node;
        while ((node = list_pop_head(&ht->buckets[i])) != NULL) {
            hashtable_entry_t* entry = (hashtable_entry_t*)node;
            if (free_entry) free_entry(entry);
        }
    }

    kfree(ht->buckets);
    kfree(ht);
    return 0;
}

int hashtable_get(hashtable_t* ht, uint64_t key, hashtable_entry_t** result) {
    if (!ht || !result) return -1;

    size_t index = key % ht->bucket_count;
    list_node_t* node = ht->buckets[index].head;
    while (node) {
        hashtable_entry_t* entry = (hashtable_entry_t*)node;
        if (entry->key == key) {
            *result = entry;
            return 0; // Success
        }
        node = node->next;
    }

    *result = NULL; // Not found
    return 0; // Success
}

int hashtable_put(hashtable_t* ht, void* entry) {
    if (!ht || !entry) return -1;

    hashtable_entry_t* ht_entry = (hashtable_entry_t*)entry;
    size_t index = ht_entry->key % ht->bucket_count;
    list_push_head(&ht->buckets[index], &ht_entry->node);

    return 0; // Success
}

int hashtable_remove(hashtable_t* ht, uint64_t key, void (*free_entry)(void*)) {
    if (!ht) return -1;

    size_t index = key % ht->bucket_count;
    list_node_t* node = ht->buckets[index].head;
    while (node) {
        hashtable_entry_t* entry = (hashtable_entry_t*)node;
        if (entry->key == key) {
            list_remove(node);
            if (free_entry) free_entry(entry);
            return 0; // Success
        }
        node = node->next;
    }

    return 0; // Not found, but not an error
}

