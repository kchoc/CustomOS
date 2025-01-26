#include "kernel/memory/kmalloc.h"
#include "kernel/terminal.h"

#define KUNIT sizeof(struct kmalloc_unit)

#define KMALLOC_STATE_FREE 0x11
#define KMALLOC_STATE_USED 0x22

static struct kmalloc_unit *head = 0;

void kmalloc_init(char *heap_start, size_t heap_size) {
    head = (struct kmalloc_unit *)heap_start;
    head->state = KMALLOC_STATE_FREE;
    head->size = heap_size;
    head->next = 0;
    head->prev = 0;
}

static void ksplit(struct kmalloc_unit *u, size_t size) {
    struct kmalloc_unit *n = (struct kmalloc_unit *)((char *)u + size);

    n->state = KMALLOC_STATE_FREE;
    n->size = u->size - size;
    n->next = u->next;
    n->prev = u;

    if (u->next)
        u->next->prev = n;

    u->size = size;
    u->next = n;
}

void *kmalloc(size_t size) {
    // Align the size to the next multiple of KUNIT
    int extra = size % KUNIT;
    if (extra)
        size += KUNIT - extra;

    // Find a free unit
    size += KUNIT;

    struct kmalloc_unit *u = head;

    while (1) {
        if (!u) {
            return 0;
        }
        if (u->state == KMALLOC_STATE_FREE && u->size >= size) {
            break;
        }
        u = u->next;
    }

    // Split the unit if it's too big
    if ((u->size - size) > 2 * KUNIT) {
        ksplit(u, size);
    }

    u->state = KMALLOC_STATE_USED;

    // Return a pointer to the memory after the unit header
    return (u + 1);
}

static void kmerge(struct kmalloc_unit *u) {
    if (!u)
        return;

    if (u->state != KMALLOC_STATE_FREE)
        return;

    if (u->next && u->next->state == KMALLOC_STATE_FREE) {
        u->size += u->next->size;
        if (u->next)
            u->next->prev = u;
        u->next = u->next->next;
    }
}

void kfree(void *ptr) {
    if (!ptr)
        return;

    struct kmalloc_unit *u = (struct kmalloc_unit *)ptr - 1;

    if (u->state != KMALLOC_STATE_USED)
        return;

    u->state = KMALLOC_STATE_FREE;

    kmerge(u);
    kmerge(u->prev);
}

void memory_usage() {
    struct kmalloc_unit *u = head;
    int i = 0;
    while (u) {
        printf("Block %d: %d bytes, %s\n", i, u->size, u->state == KMALLOC_STATE_FREE ? "free" : "used");
        u = u->next;
        i++;
    }
}