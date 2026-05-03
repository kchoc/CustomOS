#include "vm_fault.h"
#include "types.h"
#include "vm_page.h"
#include "vm_region.h"

#include <machine/pmap.h>

#include <kern/errno.h>

int vm_fault(vm_space_t* space, uintptr_t addr, vm_prot_t fault_type)
{
    vm_region_t* region = vm_region_lookup(space, addr);

    if (!region) {
        // printf("vm_fault: No region found for address 0x%08x\n", addr);
        return -1; // invalid access
    }
    if (!(region->prot & fault_type))
        return -1; // protection fault

    uintptr_t page_addr = addr & ~(PAGE_SIZE - 1);
    size_t    offset    = (page_addr - region->base) + region->offset;

    struct vm_page* page = vm_page_lookup(region->object, offset);

    if (!page) {
        page = vm_page_allocate(region->object, offset);
        if (!page)
            return -1;

        pmap_enter(space->arch, page_addr, page->phys_addr, region->prot, 0);

        // Check for copy-on-write
        if ((fault_type & VM_PROT_WRITE) && (region->prot & VM_PROT_WRITE) &&
            (region->flags & VM_REG_F_PRIVATE) && vm_object_supports_cow(region->object->type)) {

            printf("vm_fault: COW fault for address 0x%08x, page address 0x%08x, offset 0x%08x\n", addr, page_addr, offset);

            vm_page_t* parent_page = vm_page_get_cow_page(region->object, offset);
            if (IS_ERR(parent_page)) return (int)parent_page;

            printf("vm_fault: Parent page phys addr 0x%08x\n", parent_page->phys_addr);

            // Allocate a temp page for the copy
            void* temp_page = kvm_alloc(PAGE_SIZE, VM_PROT_READ | VM_PROT_WRITE, 0);
            if (IS_ERR(temp_page)) {
                return (int)temp_page;
            } 

            pmap_enter(space->arch, temp_page, parent_page->phys_addr, region->prot, 0);

            memcpy((void*)page_addr, temp_page, PAGE_SIZE);

            // if (0xBFFFF000 < addr && addr < 0xC0000000) {
                // printf("vm_fault: COW fault for address 0x%08x, parent phys addr 0x%08x\n", page_addr,
                       // parent_page->phys_addr);
                // for (int i = 0; i < 16; i++) {
                    // printf("Data at offset %d: 0x%02x\n", i, *((uint32_t*)0xBFFFFF00 + i * 4));
                // }
            // }

            pmap_remove(space->arch, temp_page, temp_page + PAGE_SIZE);
            
            kvm_free(temp_page, PAGE_SIZE);

            return 0;
        }
    }

    // TODO: FIX FLAGS
    pmap_enter(space->arch, page_addr, page->phys_addr, region->prot, 0);

    return 0;
}

