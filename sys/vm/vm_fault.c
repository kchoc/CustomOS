#include "vm_fault.h"
#include "machine/pmap.h"
#include "vm_region.h"
#include "vm_page.h"
#include "types.h"

int vm_fault(vm_space_t *space,
             uintptr_t addr,
             vm_prot_t fault_type)
{
    vm_region_t *region = vm_region_lookup(space, addr);

    if (!region)
        return -1; // invalid access

    if (!(region->prot & fault_type))
        return -1; // protection fault

    uintptr_t page_addr = addr & ~(PAGE_SIZE - 1);
    size_t offset =
        (page_addr - region->base) + region->offset;

    struct vm_page *page = vm_page_lookup(region->object, offset);

    if (!page) {
        page = vm_page_allocate(region->object, offset);
        if (!page)
            return -1;
    }

    pmap_enter(&space->arch, page_addr, page->phys_addr, region->prot, 0);

    return 0;
}