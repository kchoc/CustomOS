#include "vm_region.h"
#include "vm_space.h"
#include "vm_object.h"
#include "layout.h"

#include <machine/pmap.h>
#include "kmalloc.h"

#include <kern/terminal.h>
#include <kern/errno.h>

void vm_region_inc_ref(vm_region_t *region) {
	__sync_fetch_and_add(&region->ref_count, 1);
}

void vm_region_dec_ref(vm_region_t *region) {
	if (__sync_sub_and_fetch(&region->ref_count, 1) == 0) {
		// Free the region and its object if this was the last reference
		pmap_remove(vm_space_from_region(region)->arch, region->base, region->end);
		vm_object_dec_ref(region->object);
		kfree(region);
	}
}

bool vm_region_overlaps(vm_region_t *region, uintptr_t start, uintptr_t end) {
	return !(start >= region->end || end <= region->base);
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

vaddr_t vm_find_free_region(vm_space_t *space, size_t size, vm_region_flags_t flags) {
	uintptr_t last_end = (flags & VM_REG_F_KERNEL) ? KERNEL_BASE : 0;
	list_node_t *node;
	list_for_each(node, &space->regions) {
		vm_region_t *region = list_node_to_region(node);
		if (last_end < region->base && region->base - last_end >= size) {
			return last_end; // Found a gap large enough for the new region
		}
		last_end = region->end;
	}

	// Check for space after the last region
	if (UINTPTR_MAX - last_end >= size) {
		return last_end;
	}

	return -ENOMEM; // No suitable free region found
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

vm_region_t* vm_create_region(vm_space_t *space, uintptr_t* addr, size_t size, vm_object_t* object, vm_ooffset_t offset, vm_prot_t prot, vm_region_flags_t flags) {
    vm_region_t *region = kmalloc(sizeof(vm_region_t));
    if (!region) return ERR_PTR(-ENOMEM);

    if (object == NULL) {
    	object = vm_object_create(NULL);
    } else {
    	object->ref_count++;
    }

    region->ref_count = 1;

    // TODO: Allow 0 to be mapped
    if (addr && *addr != VM_REGION_ALLOCATE_ADDR) {
		region->base = *addr;
		if (vm_region_lookup_range(space, region->base, size)) {
			vm_region_dec_ref(region); // This will free the region since its ref count is 1 and also free the object
			return ERR_PTR(-EEXIST); // Overlap detected
		}

	} else {
		region->base = vm_find_free_region(space, size, flags);
		if (IS_ERR(region->base)) {
			vm_region_dec_ref(region); // This will free the region since its ref count is 1 and also free the object
			return ERR_PTR(region->base); // No suitable free region found
		}
		if (addr) *addr = region->base;
	}

    region->end   = region->base + size;
    region->prot  = prot;
    region->flags = flags;
    region->object = object;
    region->offset = offset;

    vm_region_t* new_region = vm_region_insert(space, region);
    if (IS_ERR(new_region)) {
		vm_region_dec_ref(region); // This will free the region since its ref count is 1 and also free the object
		return new_region; // error code
	}

    return new_region;
}

vm_region_t *vm_region_fork(vm_region_t *parent) {
    if (!parent) return ERR_PTR(-EINVAL);

    vm_region_t *child = kmalloc(sizeof(vm_region_t));
    if (!child) return ERR_PTR(-ENOMEM);

    *child = *parent;           // shallow copy
    child->ref_count = 1;

    bool private = !(parent->flags & VM_REG_F_SHARED);
    bool writable = parent->prot & VM_PROT_WRITE;
    bool cow_capable = vm_object_supports_cow(parent->object->type);

    if (private && writable && cow_capable) {
        vm_object_t *parent_shadow = vm_object_create_shadow(parent->object, 0);
        if (IS_ERR(parent_shadow)) {
            kfree(child);
            return ERR_PTR(-ENOMEM);
        }

        vm_object_t *child_shadow = vm_object_create_shadow(parent->object, 0);
        if (IS_ERR(child_shadow)) {
            vm_object_dec_ref(parent_shadow);
            kfree(child);
            return ERR_PTR(-ENOMEM);
        }
        vm_object_dec_ref(parent->object); // The parent region will now point to the shadow object, so we need to decrement the ref count of the original object

        parent->object = parent_shadow;
        child->object  = child_shadow;

        pmap_protect(vm_space_from_region(parent)->arch, parent->base, parent->end, parent->prot & ~VM_PROT_WRITE);
    } else {
        vm_object_inc_ref(parent->object);
        child->object = parent->object;

        // If the region is shared, we need to make sure the child region is also marked as shared
        if (parent->flags & VM_REG_F_SHARED)
			child->flags |= VM_REG_F_SHARED;
    }

    // Pmap entries will be copied on demand when the child process tries to access the region
	// TODO: COPY MAPPINGS

    return child;
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
	region->object->ref_count++; // Increment ref count for the shared object
	new_region->offset = region->offset + (addr - region->base);
	new_region->ref_count = 1;

	region->end = addr;

	list_insert_after(&region->node, &new_region->node);

	return new_region;
}

vm_region_t* vm_region_merge(vm_region_t *region, vm_region_t *other) {
	if (!vm_region_overlaps(region, other->base, other->end) &&
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

void vm_region_protect(vm_region_t *region, vm_prot_t new_prot) {
	region->prot = new_prot;
	pmap_protect(vm_space_from_region(region)->arch, region->base, region->end, new_prot);
}

void vm_region_flag(vm_region_t *region, vm_region_flags_t flags) {
	region->flags = flags;
}

int vm_region_rebase(vm_region_t *region, uintptr_t new_addr) {
	if (new_addr > region->end) return -EINVAL; // Invalid new base address

	vm_region_t *prev = GET_PREV_REGION(region);
	if (prev && vm_region_overlaps(prev, new_addr, region->end))
		return -EEXIST; // Overlap with previous region

	region->base = new_addr;

	return 0; // Success
}

int vm_region_resize(vm_region_t *region, uintptr_t new_end) {
	if (new_end <= region->base) return -EINVAL; // Invalid new end address

	vm_region_t *next = GET_NEXT_REGION(region);
	if (next && vm_region_overlaps(next, region->base, new_end))
		return -EEXIST; // Overlap with next region

	region->end = new_end;

	return 0; // Success
}

int vm_region_remap(vm_region_t *region, uintptr_t new_addr, size_t new_size) {
	if (new_size == 0) return -EINVAL; // Invalid new size

	uintptr_t new_end = new_addr + new_size;
	if (new_end <= new_addr) return -EINVAL; // Invalid new end address

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
		return (int)new_region; // Return the error code
	}

	return 0; // Success
}

void vm_region_destroy(vm_region_t *region) {
	list_remove(&region->node);
	vm_region_dec_ref(region); // This will free the region and its object if this was the last reference
}
