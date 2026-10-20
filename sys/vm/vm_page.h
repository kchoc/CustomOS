#ifndef VM_PAGE_H
#define VM_PAGE_H

#include "types.h"
#include "vm_object.h"

#include <kern/spinlock.h>

#include <list.h>

typedef struct vm_page vm_page_t;

typedef enum vm_page_flags {
    VM_PAGE_FLAG_FREE        = 0x0,
    VM_PAGE_FLAG_ALLOCATED   = 0x1,
    VM_PAGE_FLAG_AWAITING_IO = 0x2,
    VM_PAGE_FLAG_BUSY        = 0x4,
} vm_page_flags_t;

typedef struct vm_page {
    paddr_t      phys_addr;
    vm_ooffset_t offset;

    vm_page_flags_t state;

    spinlock_t lock;
} vm_page_t;

vm_page_t* vm_page_lookup(vm_object_t* obj, vm_ooffset_t offset);
vm_page_t* vm_page_allocate(vm_object_t* obj, vm_ooffset_t offset);
void       vm_page_free(vm_page_t* page);

#endif // VM_PAGE_H
