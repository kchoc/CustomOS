#include "vm_fault.h"
#include "types.h"
#include "vm_map.h"
#include "vm_page.h"
#include "vm_pager.h"
#include "vm_region.h"
#include <kern/errno.h>
#include <kern/panic.h>
#include <kern/rwlock.h>
#include <kern/terminal.h>
#include <machine/pmap.h>
#include <string.h>
#include <sys/pcpu.h>

int vm_fault(vm_space_t* space, uintptr_t addr, vm_prot_t fault_type)
{
    vm_region_t* region;

    region = vm_region_lookup(space, addr, rwlock_read_lock);

    if (!region) {
        vm_space_debug(space);
        printf("vm_fault: No region found for address 0x%08x (PID=%d)\n", addr,
               get_proc_from_thread(PCPU_GET(current_thread))->pid);
        PANIC_DUMP_REGISTERS(PCPU_GET(current_thread)->trapframe);
        return -1; // invalid access
    }

    // Im kinda lazy and want to avoid missing unlocks so im going to replace the lock with a scope
    // lock that unlocks when the function returns
    WITH_READ_LOCK(region->lock)

    rwlock_read_unlock(&region->lock);

    if (!(region->prot & fault_type))
        return -1; // protection fault

    printf("vm_fault: Handling fault for address 0x%08x (PID=%d)\n", addr,
           get_proc_from_thread(PCPU_GET(current_thread))->pid);

    uintptr_t page_addr = addr & ~(PAGE_SIZE - 1);
    size_t    offset    = (page_addr - region->base) + region->offset;

    vm_object_t* obj = region->object;
    vm_page_t*   page;
    vm_page_t*   new_page;
    uintptr_t    shadow_addr;

    // If the page is already present but read-only, we need to check for copy-on-write (COW)
    // conditions.
    shadow_addr = pmap_extract(space->arch, page_addr);
    if (!IS_ERR(shadow_addr) && (fault_type & VM_PROT_WRITE))
        goto cow;

    while (obj) {
        page = vm_page_lookup(obj, offset);
        if (page)
            goto found_page;

        int res = obj->pager->ops->get_page(obj, offset, &page);
        if (res == 0 && page)
            goto found_page;

        if (!obj->shadow)
            break; // no more shadow objects to check

        offset += obj->shadow_offset;
        obj = obj->shadow;
    }

    // Page not found in any object, allocate a new page
    new_page = vm_page_allocate(region->object, offset);
    if (!new_page)
        return -ENOMEM;

    pmap_enter(space->arch, page_addr, new_page->phys_addr, region->prot, 0);
    return 0;

found_page:
    // Check for COW
    if (page->state & VM_PAGE_FLAG_COW) {
        if (!(fault_type & VM_PROT_WRITE)) {
            // Read fault on a COW page, just map it as read-only (dont allocate a new page)
            pmap_enter(space->arch, page_addr, page->phys_addr, region->prot & ~VM_PROT_WRITE, 0);
            return 0;
        }

        shadow_addr = page->phys_addr;
        goto cow; // Write fault: need to allocate a new page and copy the contents
    }

    pmap_enter(space->arch, page_addr, page->phys_addr, region->prot, 0);

    return 0;

cow:
    new_page = vm_page_allocate(region->object, offset);
    if (!new_page)
        return -ENOMEM;

    vaddr_t temp_page = (vaddr_t)kvm_alloc(PAGE_SIZE, VM_PROT_READ | VM_PROT_WRITE, 0);
    if (IS_ERR(temp_page))
        return (int)temp_page;

    pmap_enter(space->arch, temp_page, shadow_addr, VM_PROT_READ, 0);
    pmap_enter(space->arch, page_addr, new_page->phys_addr, region->prot, 0);
    memcpy((void*)page_addr, (void*)temp_page, PAGE_SIZE);
    pmap_remove(space->arch, temp_page, temp_page + PAGE_SIZE);
    kvm_free((void*)temp_page, PAGE_SIZE);

    END_WITH_READ_LOCK;

    return 0;
}
