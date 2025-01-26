#ifndef KMALLOC_H
#define KMALLOC_H

#include <stddef.h>
#include <stdint.h>

struct kmalloc_unit {
    uint8_t state;
    size_t size;
    struct kmalloc_unit *next;
    struct kmalloc_unit *prev;
};

void* kmalloc(size_t size);
void kfree(void* ptr);

void kmalloc_init(char *heap_start, size_t heap_size);
void memory_usage(void);

#endif // KMALLOC_H