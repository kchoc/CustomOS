#include <vm/kmalloc.h>
#include <kern/spinlock.h>
#include <kern/terminal.h>

#define KUNIT sizeof(struct kmalloc_unit)
#define KMALLOC_STATE_FREE 0x11
#define KMALLOC_STATE_USED 0x22

static struct kmalloc_unit *head = 0;
static spinlock_t kmalloc_lock = 0;

int kmalloc_init(char *heap_start, size_t heap_size) {
    head = (kmalloc_unit_t *)heap_start;
    head->state = KMALLOC_STATE_FREE;
    head->size = heap_size;
    head->next = 0;
    head->prev = 0;
    return 0;
}

static void ksplit(kmalloc_unit_t *u, size_t size) {
    if (u->size <= size + KUNIT)
        return;

    kmalloc_unit_t *n = (kmalloc_unit_t *)((char *)u + size);

    n->state = KMALLOC_STATE_FREE;
    n->size = u->size - size;
    n->next = u->next;
    n->prev = u;

    if (u->next)
        u->next->prev = n;

    u->size = size;
    u->next = n;
}

static void kmerge(kmalloc_unit_t *u) {
    if (!u || u->state != KMALLOC_STATE_FREE)
        return;

    if (u->next && u->next->state == KMALLOC_STATE_FREE) {
        u->size += u->next->size;
        if (u->next) u->next->prev = u;
        u->next = u->next->next;
    }

    if (u->prev && u->prev->state == KMALLOC_STATE_FREE) {
        u->prev->size += u->size;
        u->prev->next = u->next;
        if (u->next) u->next->prev = u->prev;
    }
}

static void *kmalloc_unsafe(size_t size) {
    size_t needed = size + KUNIT;
    size_t rem = needed % KUNIT;
    if (rem)
        needed += KUNIT - rem;

    struct kmalloc_unit *u = head;

    while (u && !(u->state == KMALLOC_STATE_FREE && u->size >= needed)) {
        u = u->next;
    }

    if (!u) {
        return 0; // No suitable block found
    }

    ksplit(u, needed);
    u->state = KMALLOC_STATE_USED;

    return (void *)(u + 1);
}

void *kmalloc(size_t size) {
    spin_lock(&kmalloc_lock);
    void *ptr = kmalloc_unsafe(size);
    spin_unlock(&kmalloc_lock);
    return ptr;
}

void *kmalloc_aligned(size_t size, size_t alignment) {
    spin_lock(&kmalloc_lock);
    size_t extra = alignment + KUNIT;
    void* raw_ptr = kmalloc_unsafe(size + extra);

    if (!raw_ptr) {
        spin_unlock(&kmalloc_lock);
        return 0;
    }

    uintptr_t raw_addr = (uintptr_t)raw_ptr;
    uintptr_t aligned_addr = (raw_addr + extra - 1) & ~(alignment - 1);

    if (aligned_addr == raw_addr) {
        spin_unlock(&kmalloc_lock);
        return raw_ptr;
    }

    kmalloc_unit_t *u = (kmalloc_unit_t *)raw_ptr - 1;

    size_t prefix_size = aligned_addr - raw_addr;
    if (prefix_size >= KUNIT + 1) {
        ksplit(u, prefix_size);
        u = u->next;
    }
    
    ksplit(u, size + KUNIT);
    u->state = KMALLOC_STATE_USED;

    void *ptr = (void *)(u + 1);

    spin_unlock(&kmalloc_lock);
    return ptr;
}

void kfree(void *ptr) {
    if (!ptr) return;

    spin_lock(&kmalloc_lock);

    kmalloc_unit_t *u = (kmalloc_unit_t *)ptr - 1;

    if (u->state == KMALLOC_STATE_USED) {
        u->state = KMALLOC_STATE_FREE;
        kmerge(u);
    }

    spin_unlock(&kmalloc_lock);
}

void memory_usage() {
    spin_lock(&kmalloc_lock);
    kmalloc_unit_t *u = head;
    int i = 0;
    while (u) {
        printf("Block %d: %d bytes, %s\n", i, u->size, u->state == KMALLOC_STATE_FREE ? "free" : "used");
        u = u->next;
        i++;
    }
    spin_unlock(&kmalloc_lock);
}