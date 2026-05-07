#ifndef VM_PAGER_H
#define VM_PAGER_H

#include "types.h"

typedef struct vm_object vm_object_t;
typedef struct vm_page   vm_page_t;

typedef struct vm_pager_ops {
    int (*get_page)(vm_object_t* obj, vm_ooffset_t offset, vm_page_t** page);
    int (*alloc_page)(vm_object_t* obj, vm_ooffset_t offset, vm_page_t** page);
    int (*put_page)(vm_object_t* obj, vm_page_t* page);
    bool (*has_page)(vm_object_t* obj, vm_ooffset_t offset);
    int (*lookup_page)(vm_object_t* obj, vm_ooffset_t offset, vm_page_t** page);
    void (*destroy)(vm_object_t* obj);
} vm_pager_ops_t;

typedef struct vm_pager {
    vm_pager_ops_t* ops;
    void*           data; // Pager-specific data (e.g. file handle for vnode pager)
} vm_pager_t;

vm_pager_t* vm_pager_create(vm_pager_ops_t* ops, void* data);
void        vm_pager_destroy(vm_pager_t* pager);

#define DECLARE_VM_PAGER_OPS(name)                                                \
    int name##_pager_get_page(vm_object_t* obj, vm_ooffset_t offset, vm_page_t** page); \
    int name##_pager_alloc_page(vm_object_t* obj, vm_ooffset_t offset, vm_page_t** page); \
    int name##_pager_put_page(vm_object_t* obj, vm_page_t* page);                       \
    bool name##_pager_has_page(vm_object_t* obj, vm_ooffset_t offset);                  \
    int name##_pager_lookup_page(vm_object_t* obj, vm_ooffset_t offset, vm_page_t** page); \
    void name##_pager_destroy(vm_object_t* obj);                                        \
    extern vm_pager_ops_t name##_pager_ops;

DECLARE_VM_PAGER_OPS(anon)

#endif // VM_PAGER_H
