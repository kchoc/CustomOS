#include "vm_space.h"
#include "kmalloc.h"
#include "pmap.h"
#include "types.h"
#include "vm_map.h"
#include "vm_region.h"
#include "vm_temp.h"

#include <sys/pcpu.h>

#include <kern/errno.h>
#include <kern/panic.h>
#include <kern/terminal.h>

#include <list.h>

vm_space_t kvm_space_store = {
    .regions = LIST_INIT, .pmap = kernel_pmap, .regions_lock = RWLOCK_INITIALIZER};

int kvm_space_init()
{
    pmap_init();

    pcpus[0].vmspace = kvm_space;

    // Allocate a region for the kernel's virtual address space (1MB for now, can be expanded later)
    vaddr_t virt = KERNEL_BASE;
    int     res  = vm_map_anon(kvm_space, &virt, 0x00400000, VM_PROT_READ | VM_PROT_WRITE,
                               VM_REG_F_KERNEL, VM_MAP_F_FIXED);
    if (res)
        return res;

    res = vm_temp_init();
    if (res)
        return res;

    // TODO: Need to bookkeep the vm_pages from the bootloader
    return 0;
}

vm_space_t* vm_space_create()
{
    vm_space_t* space = kmalloc(sizeof(vm_space_t));
    if (!space)
        return ERR_PTR(-ENOMEM);

    space->regions_lock = RWLOCK_INITIALIZER;
    list_init(&space->regions, 0);

    space->pmap = pmap_create();
    if (IS_ERR(space->pmap)) {
        kfree(space);
        return ERR_PTR(-ENOMEM);
    }

    return space;
}

vm_space_t* vm_space_fork(vm_space_t* parent)
{
    if (!parent)
        return ERR_PTR(-EINVAL);

    vm_space_t* child = vm_space_create();
    if (IS_ERR(child))
        return ERR_PTR(-ENOMEM);

    WITH_READ_LOCK(parent->regions_lock)
    {
        // Copy the regions list by forking each region.
        for (list_node_t* node = parent->regions.head; node; node = node->next) {
            vm_region_t* reference_region = list_node_to_region(node);
            vm_region_t* child_region     = vm_region_fork(reference_region);
            if (IS_ERR(child_region)) {
                vm_space_destroy(child); // Clean up the child space and all regions created so far
                return ERR_PTR(-ENOMEM);
            }
            // Insert the child region into the child's region list
            list_push_tail(&child->regions, &child_region->node);
        }
    }

    return child;
}

void vm_space_destroy(vm_space_t* space)
{
    if (!space)
        return;

    WITH_WRITE_LOCK(space->regions_lock)
    {
        while (space->regions.head) {
            vm_region_t* region = list_node_to_region(space->regions.head);
            vm_region_destroy(region);
        }
    }

    pmap_destroy(space->pmap);

    kfree(space);
}

void vm_space_clean(vm_space_t* space)
{
    if (!space)
        return;

    WITH_WRITE_LOCK(space->regions_lock)
    {
        // Similar to destroy, but kernel regions should not be freed, and the vm_space itself
        // should not be freed. Only user regions should be cleaned
        list_node_t* node = space->regions.head;
        while (node) {
            vm_region_t* region = list_node_to_region(node);
            node                = node->next;
            if (!(region->flags & VM_REG_F_KERNEL)) {
                pmap_remove(space->pmap, region->base, region->end);
                vm_region_destroy(region);
            }
        }
    }
}

void vm_space_activate(vm_space_t* space)
{
    if (!space)
        return;
    pmap_activate(space->pmap);
}

void vm_space_debug(vm_space_t* space)
{
    if (!space)
        return;

    WITH_READ_LOCK(space->regions_lock)
    {
        printf("VM Space at %p\n", space);
        printf("  Regions:\n");
        list_node_t* node;
        list_for_each(node, &space->regions)
        {
            vm_region_t* region = list_node_to_region(node);
            printf("    Region at %p: base=0x%08x, end=0x%08x, prot=%d, flags=%d\n", region,
                   region->base, region->end, region->prot, region->flags);
        }
    }
    // pmap_debug(space->pmap);
}
