#include "vm_temp.h"
#include "layout.h"
#include "pmap.h"
#include "vm_map.h"
#include "vm_space.h"
#include <kern/errno.h>
#include <kern/terminal.h>
#include <string.h>

uint32_t   vm_temp_bitmap = 0; // Bitmap to track temporary page usage (up to 32 pages)
spinlock_t vm_temp_lock   = SPINLOCK_INITIALIZER;

int vm_temp_init()
{
    // Ensure the temporary mapping region is allocated in the kernel's virtual address space
    vaddr_t virt = VM_TEMP_START;
    int ret = vm_map_anon(kvm_space, &virt, VM_TEMP_PAGES * PAGE_SIZE, VM_PROT_READ | VM_PROT_WRITE,
                          VM_REG_F_KERNEL, VM_MAP_F_FIXED);
    if (IS_ERR(ret))
        return ret;

    return 0;
}

void* vm_temp_map_page(paddr_t phys_addr, vm_prot_t prot)
{
    int index = -1;

    WITH_SPINLOCK(vm_temp_lock)
    {
        for (int i = 0; i < VM_TEMP_PAGES; i++) {
            if (!(vm_temp_bitmap & (1 << i))) {
                index = i;
                vm_temp_bitmap |= (1 << i); // Mark the page as used
                break;
            }
        }
    }

    if (index == -1) {
        return NULL; // No free temporary pages available
    }

    vaddr_t virt_addr = VM_TEMP_START + index * PAGE_SIZE;

    pmap_enter(kernel_pmap, virt_addr, phys_addr, prot, 0);

    return (void*)virt_addr;
}

void vm_temp_unmap_page(void* virt_addr)
{
    if (!virt_addr) {
        return;
    }

    vaddr_t addr = (vaddr_t)virt_addr;
    if (addr < VM_TEMP_START || addr >= VM_TEMP_START + VM_TEMP_PAGES * PAGE_SIZE) {
        return;
    }

    int index = (addr - VM_TEMP_START) / PAGE_SIZE;

    pmap_remove(kernel_pmap, addr, addr + PAGE_SIZE);

    WITH_SPINLOCK(vm_temp_lock)
    {
        vm_temp_bitmap &= ~(1 << index); // Mark the page as free
    }
}
