#include "vm_region.h"
#include "kmalloc.h"
#include "layout.h"
#include "types.h"
#include "vm_object.h"
#include "vm_space.h"

#include <machine/pmap.h>

#include <kern/errno.h>
#include <kern/panic.h>
#include <kern/terminal.h>

/* ==============================
 * Miscellaneous helper functions
 * ============================== */

static inline bool vm_region_overlaps(vm_region_t* region, uintptr_t start, uintptr_t end)
{
    return !(start >= region->end || end <= region->base);
}

vm_region_t* vm_region_lookup(vm_space_t* space, uintptr_t addr, lock_func_t lock_func)
{
    WITH_READ_LOCK(space->regions_lock)

    list_node_t* node;
    list_for_each(node, &space->regions)
    {
        vm_region_t* region = list_node_to_region(node);
        if (addr >= region->base && addr < region->end) {
            if (lock_func)
                lock_func(&region->lock);
            return region;
        }
    }

    END_WITH_READ_LOCK;

    return NULL;
}

vaddr_t vm_find_free_region(vm_space_t* space, size_t size, vm_region_flags_t flags)
{
    uintptr_t last_end = 0;
    if (flags & VM_REG_F_DEVICE)
        last_end = DEVICE_BASE;
    else if (flags & VM_REG_F_KERNEL)
        last_end = KERNEL_BASE;

    list_node_t* node;
    list_for_each(node, &space->regions)
    {
        vm_region_t* region = list_node_to_region(node);
        if (last_end < region->base && region->base - last_end >= size)
            return last_end; // Found a gap large enough for the new region

        last_end = region->end;
    }

    // Check for space after the last region
    if (ADDRESS_LIMIT - last_end >= size)
        return last_end;

    return -ENOMEM; // No suitable free region found
}

vm_region_t* vm_region_lookup_range(vm_space_t* space, uintptr_t addr, size_t size)
{
    list_node_t* node;
    list_for_each(node, &space->regions)
    {
        vm_region_t* region = list_node_to_region(node);
        if (addr < region->end && addr + size > region->base)
            return region;
    }

    return NULL;
}

void vm_region_free_range(vm_space_t* space, uintptr_t addr, size_t size)
{
    WITH_WRITE_LOCK(space->regions_lock)

    list_node_t* node = space->regions.head;
    while (node) {
        vm_region_t* region = list_node_to_region(node);
        node                = node->next;

        if (region->end <= addr)
            continue; // Region is completely before the range

        if (region->base >= addr + size)
            break; // Region is completely after the range

        if (region->base < addr) {
            if (region->end > addr + size) {
                // The range is in the middle of the region, split it into two
                vaddr_t      new_base = addr + size;
                vm_region_t* new_region =
                    vm_region_create(space, &new_base, region->end - (addr + size), region->object,
                                     region->offset + (addr + size - region->base), region->prot,
                                     region->flags, VM_MAP_F_NONE);
                if (IS_ERR(new_region)) {
                    // Handle error (e.g., log it, panic, etc.)
                    PANIC("Failed to create new region during free range");
                }

                region->end = addr; // Adjust the end of the original region
            }
            else {
                // The range overlaps with the end of the region, adjust the end
                region->end = addr;
            }
        }
        else {
            if (region->end > addr + size) {
                // The range overlaps with the start of the region, adjust the base and offset
                region->offset += (addr + size - region->base);
                region->base = addr + size;
            }
            else {
                // The range completely covers the region, remove it
                vm_region_destroy(region);
            }
        }
    }
    END_WITH_WRITE_LOCK;
}

void vm_region_protect_range(vm_space_t* space, uintptr_t addr, size_t size, vm_prot_t new_prot)
{
    WITH_WRITE_LOCK(space->regions_lock)

    list_node_t* node = space->regions.head;
    while (node) {
        vm_region_t* region = list_node_to_region(node);
        node                = node->next;

        if (region->end <= addr)
            continue; // Region is completely before the range

        if (region->base >= addr + size)
            break; // Region is completely after the range

        // The region overlaps with the specified range
        if (region->base < addr) {
            // Split the region into two parts: before and after the specified range
            vaddr_t      new_base   = addr;
            vm_region_t* new_region = vm_region_create(
                space, &new_base, region->end - addr, region->object,
                region->offset + (addr - region->base), new_prot, region->flags, VM_MAP_F_NONE);
            if (IS_ERR(new_region)) {
                // Handle error (e.g., log it, panic, etc.)
                PANIC("Failed to create new region during protect range");
            }

            region->end = addr; // Adjust the end of the original region
        }
        else {
            // Adjust the protection of the overlapping part
            region->prot = new_prot;
        }
    }

    END_WITH_WRITE_LOCK;
}

vm_region_t* vm_region_create(vm_space_t* space, vaddr_t* addr, size_t size, vm_object_t* object,
                              vm_ooffset_t offset, vm_prot_t prot, vm_region_flags_t flags,
                              vm_map_flags_t map_flags)
{
    vm_region_t* region = kmalloc(sizeof(vm_region_t));
    if (!region)
        return ERR_PTR(-ENOMEM);

    if (object == NULL)
        object = vm_object_create_anon();
    else
        vm_object_inc_ref(object);

    region->object = object;

    WITH_WRITE_LOCK(space->regions_lock)

    // TODO: Allow 0 to be mapped
    if (addr && (map_flags & VM_MAP_F_FIXED)) {
        region->base = *addr;
        if (vm_region_lookup_range(space, region->base, size)) {
            vm_object_dec_ref(object);
            kfree(region);
            return ERR_PTR(-EEXIST); // Overlap detected
        }
    }
    else {
        region->base = vm_find_free_region(space, size, flags);
        if (IS_ERR(region->base)) {
            vm_object_dec_ref(object);
            kfree(region);
            return ERR_PTR(region->base); // No suitable free region found
        }
        if (addr)
            *addr = region->base;
    }

    region->end    = region->base + size;
    region->prot   = prot;
    region->flags  = flags;
    region->offset = offset;
    region->lock   = RWLOCK_INITIALIZER;

    vm_region_t* new_region = vm_region_insert(space, region);
    if (IS_ERR(new_region)) {
        vm_object_dec_ref(object);
        kfree(region);
        return new_region; // error code
    }

    return new_region;

    END_WITH_WRITE_LOCK;
}

vm_region_t* vm_region_fork(vm_region_t* parent)
{
    if (!parent)
        return ERR_PTR(-EINVAL);

    vm_region_t* child = kmalloc(sizeof(vm_region_t));
    if (!child)
        return ERR_PTR(-ENOMEM);

    *child = *parent; // shallow copy

    bool private     = !(parent->flags & VM_REG_F_SHARED);
    bool writable    = parent->prot & VM_PROT_WRITE;
    bool cow_capable = vm_object_supports_cow(parent->object->type);

    if (private && writable && cow_capable && !(parent->flags & VM_REG_F_KERNEL)) {
        vm_object_t* parent_shadow = vm_object_create_shadow(parent->object, 0);
        if (IS_ERR(parent_shadow)) {
            kfree(child);
            return ERR_PTR(-ENOMEM);
        }

        vm_object_t* child_shadow = vm_object_create_shadow(parent->object, 0);
        if (IS_ERR(child_shadow)) {
            vm_object_dec_ref(parent_shadow);
            kfree(child);
            return ERR_PTR(-ENOMEM);
        }

        vm_object_dec_ref(parent->object);

        parent->object = parent_shadow;
        child->object  = child_shadow;

        pmap_protect(vm_space_from_region(parent)->arch, parent->base, parent->end,
                     parent->prot & ~VM_PROT_WRITE);
    }
    else {
        vm_object_inc_ref(parent->object);
    }

    return child;
}

// Insert a region into a sorted list, merging if possible
vm_region_t* vm_region_insert(vm_space_t* space, vm_region_t* new_region)
{
    list_node_t* node;
    vm_region_t* prev = NULL;
    vm_region_t* next = NULL;

    // Find insertion point
    list_for_each(node, &space->regions)
    {
        vm_region_t* region = list_node_to_region(node);
        if (region->base > new_region->base) {
            next = region;
            break;
        }
        prev = region;
    }

    uintptr_t new_base = new_region->base;
    uintptr_t new_end  = new_region->end;

    // Check overlap with previous region
    if (prev && vm_region_overlaps(prev, new_base, new_end)) {
        return ERR_PTR(-EEXIST); // overlap detected, fail
    }

    // Check overlap with next region
    if (next && vm_region_overlaps(next, new_base, new_end)) {
        return ERR_PTR(-EEXIST);
    }

    // Try merging with previous
    if (prev && prev->end == new_base && prev->prot == new_region->prot &&
        prev->flags == new_region->flags && prev->object == new_region->object &&
        prev->offset + (prev->end - prev->base) == new_region->offset) {
        prev->end = new_region->end;
        vm_region_destroy(new_region); // Free the new region since we're merging it into prev
        new_region = prev;
    }
    else {
        // Insert new_region into list
        if (prev)
            list_insert_after(&prev->node, &new_region->node);
        else
            list_push_head(&space->regions, &new_region->node);
    }

    // Try merging with next
    if (next && new_region->end == next->base && new_region->prot == next->prot &&
        new_region->flags == next->flags && new_region->object == next->object &&
        new_region->offset + (new_region->end - new_region->base) == next->offset) {
        new_region->end = next->end;
        vm_region_destroy(next); // Free the next region since we're merging it into new_region
    }

    return new_region;
}

void vm_region_destroy(vm_region_t* region)
{
    WITH_WRITE_LOCK(region->lock)

    vm_object_dec_ref(region->object);
    list_remove(&region->node);
    kfree(region);

    END_WITH_WRITE_LOCK;
}
