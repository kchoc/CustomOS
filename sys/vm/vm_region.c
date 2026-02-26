#include "vm_region.h"
#include "vm_space.h"
#include "vm_object.h"

#include <machine/pmap.h>
#include "kmalloc.h"

#include <kern/errno.h>

void vm_region_inc_ref(vm_region_t *region) {
	__sync_fetch_and_add(&region->ref_count, 1);
}

void vm_region_dec_ref(vm_region_t *region) {
	if (__sync_fetch_and_sub(&region->ref_count, 1) == 0) {
		// Free the region and its object if this was the last reference
		vm_object_dec_ref(region->object);
		kfree(region);
	}
}

bool vm_region_overlaps(vm_region_t *region, uintptr_t addr, size_t size) {
	return !(addr >= region->end || addr + size <= region->base);
}

vm_region_t* vm_region_lookup(vm_space_t *space, uintptr_t addr) {
    list_node_t *node;
    list_for_each(node, &space->regions) {
		vm_region_t *region = list_node_to_region(node);
		if (addr >= region->base && addr < region->end) {
			return region;
		}
	}

    return NULL;
}

vm_region_t* vm_region_lookup_range(vm_space_t *space, uintptr_t addr, size_t size) {
	list_node_t *node;
	list_for_each(node, &space->regions) {
		vm_region_t *region = list_node_to_region(node);
		if (addr < region->end && addr + size > region->base) {
			return region;
		}
	}

	return NULL;
}

vm_region_t* vm_create_region(vm_space_t *space, uintptr_t addr, size_t size, vm_prot_t prot, vm_flags_t flags) {
    vm_region_t *region = kmalloc(sizeof(vm_region_t));
    if (!region) return ERR_PTR(-ENOMEM);

    vm_object_t *object = kmalloc(sizeof(vm_object_t));
    if (!object) {
        kfree(region);
        return ERR_PTR(-ENOMEM);
    }

    object->ref_count = 1;
    region->ref_count = 1;

    region->base = addr;
    region->end   = addr + size;
    region->prot  = prot;
    region->flags = flags;
    region->object = object;
    region->offset = 0;

    vm_region_t* new_region = vm_region_insert(space, region);
    if (IS_ERR(new_region)) {
		vm_region_dec_ref(region); // This will free the region since its ref count is 1 and also free the object
		return new_region; // error code
	}

    return new_region;
}

// Insert a region into a sorted list, merging if possible
vm_region_t* vm_region_insert(vm_space_t *space, vm_region_t *new_region) {
    list_node_t *node;
    vm_region_t *prev = NULL;
    vm_region_t *next = NULL;

    // Find insertion point
    list_for_each(node, &space->regions) {
        vm_region_t *region = list_node_to_region(node);
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
    if (prev && prev->end == new_base &&
        prev->prot == new_region->prot &&
        prev->flags == new_region->flags &&
        prev->object == new_region->object &&
        prev->offset + (prev->end - prev->base) == new_region->offset) 
    {
        prev->end = new_region->end;
        vm_region_dec_ref(new_region); // Free the new region since we're merging it into prev
        new_region = prev;
    } else {
        // Insert new_region into list
        if (prev)
            list_insert_after(&prev->node, &new_region->node);
        else
            list_push_head(&space->regions, &new_region->node);
    }

    // Try merging with next
    if (next &&
        new_region->end == next->base &&
        new_region->prot == next->prot &&
        new_region->flags == next->flags &&
        new_region->object == next->object &&
        new_region->offset + (new_region->end - new_region->base) == next->offset)
    {
        new_region->end = next->end;
        list_remove(&next->node);
        vm_region_dec_ref(next); // Free the next region since we're merging it into new_region
    }

    return new_region;
}

vm_region_t* vm_region_split(vm_region_t *region, uintptr_t addr) {
	if (addr <= region->base || addr >= region->end) {
		return ERR_PTR(-EINVAL); // Address out of bounds
	}

	vm_region_t *new_region = kmalloc(sizeof(vm_region_t));
	if (!new_region) return ERR_PTR(-ENOMEM);

	new_region->base = addr;
	new_region->end = region->end;
	new_region->prot = region->prot;
	new_region->flags = region->flags;
	new_region->object = region->object;
	new_region->offset = region->offset + (addr - region->base);
	new_region->ref_count = 1;

	region->end = addr;

	list_insert_after(&region->node, &new_region->node);

	return new_region;
}

vm_region_t* vm_region_merge(vm_region_t *region, vm_region_t *other) {
	if (!vm_region_overlaps(region, other->base, other->end - other->base) &&
	    region->prot == other->prot &&
	    region->flags == other->flags &&
	    region->object == other->object &&
	    region->offset + (region->end - region->base) == other->offset) 
	{
		if (region->base < other->base) {
			region->end = other->end;
		} else {
			region->base = other->base;
			region->offset = other->offset;
		}

		list_remove(&other->node);
		vm_region_dec_ref(other); // Free the other region since we're merging it into region
		return region;
	}

	return ERR_PTR(-EINVAL); // Regions cannot be merged
}

vm_region_t* vm_region_protect(vm_region_t *region, vm_prot_t new_prot) {
	region->prot = new_prot;
}

vm_region_t* vm_region_flag(vm_region_t *region, vm_flags_t flags) {
	region->flags = flags;
}

vm_region_t* vm_region_rebase(vm_region_t *region, uintptr_t new_addr) {
	if (new_addr > region->end) {
		return ERR_PTR(-EINVAL); // Invalid new base address
	}
	vm_region_t *prev = GET_PREV_REGION(region);
	if (prev && vm_region_overlaps(prev, new_addr, region->end)) {
		return ERR_PTR(-EEXIST); // Overlap with previous region
	}

	region->base = new_addr;
}

vm_region_t* vm_region_resize(vm_region_t *region, uintptr_t new_end) {
	if (new_end <= region->base) {
		return ERR_PTR(-EINVAL); // Invalid new end address
	}

	vm_region_t *next = GET_NEXT_REGION(region);
	if (next && vm_region_overlaps(next, region->base, new_end - region->base)) {
		return ERR_PTR(-EEXIST); // Overlap with next region
	}

	region->end = new_end;
}

vm_region_t* vm_region_remap(vm_region_t *region, uintptr_t new_addr, size_t new_size) {
	if (new_size == 0) {
		return ERR_PTR(-EINVAL); // Invalid new size
	}

	uintptr_t new_end = new_addr + new_size;
	if (new_end <= new_addr) {
		return ERR_PTR(-EINVAL); // Invalid new end address
	}

	uintptr_t old_base = region->base;
	uintptr_t old_end = region->end;

	list_remove(&region->node); // Temporarily remove the region from the list to avoid self-overlap during checks

	region->base = new_addr;
	region->end = new_end;

	vm_region_t* new_region = vm_region_insert(vm_space_from_region(region), region);
	if (IS_ERR(new_region)) {
		// Revert changes if insertion failed
		region->base = old_base;
		region->end = old_end;
		vm_region_insert(vm_space_from_region(region), region); // Reinsert the original region
		return new_region; // Return the error code
	}

	return new_region;
}

vm_region_t* vm_region_destroy(vm_region_t *region) {
	list_remove(&region->node);
	vm_region_dec_ref(region); // This will free the region and its object if this was the last reference
}
