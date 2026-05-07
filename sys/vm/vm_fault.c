#include "vm_fault.h"
#include "types.h"
#include "vm_page.h"
#include "vm_region.h"
#include "vm_map.h"
#include "vm_pager.h"

#include <machine/pmap.h>

#include <kern/terminal.h>
#include <kern/errno.h>

#include <sys/pcpu.h>

#include <kern/panic.h>

#include <string.h>

int vm_fault(vm_space_t* space, uintptr_t addr, vm_prot_t fault_type)
{
    vm_region_t* region = vm_region_lookup(space, addr);

    if (!region) {
        printf("vm_fault: No region found for address 0x%08x (PID=%d)\n", addr, PCPU_GET(current_thread)->proc->pid);
        PANIC_DUMP_REGISTERS(PCPU_GET(current_thread)->trapframe);
        return -1; // invalid access
    }
    if (!(region->prot & fault_type))
        return -1; // protection fault

    uintptr_t page_addr = addr & ~(PAGE_SIZE - 1);
    size_t    offset    = (page_addr - region->base) + region->offset;


    vm_page_t* page;
    int res = region->object->pager->ops->lookup_page(region->object, offset, &page);
    if (res == 0) {
        // Page already exists, just map it in
        region->object->pager->ops->get_page(region->object, offset, &page);
        if (res) return res; // failed to get page 

        pmap_enter(space->arch, page_addr, page->phys_addr, region->prot, 0);

        return 0;
    }
    // Some other error occurred during lookup (e.g. I/O error for file-backed page), return it
    if (res != -ENOENT) return res; // some other error

    // Page not found, need to allocate and check for shadow object if this is a COW mapping 
    vm_page_t* shadow_page = NULL;
    vm_object_t* current_object = region->object;
    while (current_object->shadow) {
        current_object = current_object->shadow;
        res = current_object->pager->ops->lookup_page(current_object, offset, &shadow_page);
        if (res == 0) {
            // Found a page in the shadow object, need to copy it to the parent object
            break;
        }
        if (res != -ENOENT) {
            return res; // some other error
        }
    }

    // If we found a page in a shadow object, we need to copy it to the parent object and then map it in
    if (shadow_page) {

        // If this is a read fault, we can just map the shadow page directly without copying
        if (!(fault_type & VM_PROT_WRITE)) {
            pmap_enter(space->arch, page_addr, shadow_page->phys_addr, region->prot, 0);
            return 0;
        }

        // Found a page in the shadow object, need to copy it to the parent object
        res = region->object->pager->ops->alloc_page(region->object, offset, &page);
        if (res) return res; // allocation failed

        // Map the shadow page temporarily to copy its contents
        void* temp_page = kvm_alloc(PAGE_SIZE, VM_PROT_READ | VM_PROT_WRITE, 0);
        if (IS_ERR(temp_page)) {
            return (int)temp_page;
        }

        pmap_enter(space->arch, temp_page, shadow_page->phys_addr, VM_PROT_READ, 0);

        memcpy((void*)page_addr, temp_page, PAGE_SIZE);

        pmap_remove(space->arch, temp_page, temp_page + PAGE_SIZE);
        
        kvm_free(temp_page, PAGE_SIZE);

        pmap_enter(space->arch, page_addr, page->phys_addr, region->prot, 0);

        return 0;
    }

    // No page in shadow objects, need to allocate a new page
    res = region->object->pager->ops->alloc_page(region->object, offset, &page);
    if (res) return res; // allocation failed

    pmap_enter(space->arch, page_addr, page->phys_addr, region->prot, 0);

    memset((void*)page_addr, 0, PAGE_SIZE);
    page->state &= ~VM_PAGE_FLAG_TO_BE_ZEROED;

    return 0;
}

