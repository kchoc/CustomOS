#ifndef VM_PAGE_H
#define VM_PAGE_H

#include "types.h"
#include "vm_object.h"

#include <kern/spinlock.h>

#include <list.h>

typedef struct vm_page vm_page_t;

typedef enum vm_page_flags {
    VM_PAGE_FLAG_FREE      = 0x0,
    VM_PAGE_FLAG_ALLOCATED = 0x1,
    VM_PAGE_FLAG_COW       = 0x2, // Copy-on-write page
} vm_page_flags_t;

typedef int (*vm_page_fault_handler_t)(vm_object_t* obj, vm_ooffset_t offset, vm_page_t* page,
                                       vm_prot_t fault_type);

typedef struct vm_page {
    paddr_t      phys_addr; // Physical address of the page
    vm_ooffset_t offset;    // Offset within the object

    vm_page_flags_t         state;
    vm_page_fault_handler_t fault_handler; // Optional custom fault handler for this page

    bool       dirty; // Whether the page has been modified
    spinlock_t lock;
} vm_page_t;

vm_page_t* vm_page_lookup(vm_object_t* obj, size_t offset);
vm_page_t* vm_page_allocate(vm_object_t* obj, size_t offset);
void       vm_page_free(vm_page_t* page);

#endif // VM_PAGE_H
