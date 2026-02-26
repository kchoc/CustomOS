#ifndef KMALLOC_H
#define KMALLOC_H

#include <inttypes.h>
#include <stddef.h>
#include <string.h>

#define KMALLOC(var, type, size)                \
    type *var __attribute__((cleanup(kfreep)))  \
    = kmalloc(size);

#define KMALLOC_RET(var, type, size, ret)       \
    KMALLOC(var, type, size);                   \
    if (!var) return ret;

#define KMALLOC_OR_GOTO(var, type, size, label) \
    KMALLOC(var, type, size);                   \
    if (!var) goto label;

typedef struct kmalloc_unit {
    uint8_t state;
    size_t size;
    struct kmalloc_unit *next;
    struct kmalloc_unit *prev;
} kmalloc_unit_t;

void* kmalloc(size_t size);
void* kmalloc_aligned(size_t size, size_t alignment);
void kfree(void* ptr);

int kmalloc_init(char *heap_start, size_t heap_size);
void memory_usage(void);

static inline void kfreep(void *ptr) {
    void **p = (void **)ptr;
    if (*p) {
        kfree(*p);
        *p = NULL;
    }
}

#endif // KMALLOC_H