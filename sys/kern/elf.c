#include "elf.h"
#include "process.h"
#include "terminal.h"
#include <fs/iname.h>
#include <fs/vfs.h>
#include <kern/errno.h>
#include <sys/pcpu.h>
#include <vm/kmalloc.h>
#include <vm/vm_map.h>
#include <vm/vm_object.h>
#include <vm/vm_space.h>

#include <stdint.h>

// REQUIRES that the thread be the current thread
int load_elf(const char* filepath, thread_t* thread)
{
    int res;

    if (!filepath || !thread || thread != PCPU_GET(current_thread))
        return -1;

    vnode_t* vnode;
    res = iname_lookup(filepath, NULL, &vnode);
    if (res)
        return -1;

    vm_object_t* obj = vnode_get_object(vnode);
    if (IS_ERR(obj)) {
        vnode_dec_ref(vnode);
        return -1;
    }

    vnode_dec_ref(vnode);

    file_t* file = vfs_open(filepath, 0, 0x1);
    if (!file) {
        vm_object_dec_ref(obj);
        return -1;
    }

    Elf32_Ehdr eh;
    res = vfs_read(file, (uint8_t*)&eh, sizeof(Elf32_Ehdr), 0);

    /* Basic ELF validation */
    if (eh.e_ident[0] != 0x7F || eh.e_ident[1] != 'E' || eh.e_ident[2] != 'L' ||
        eh.e_ident[3] != 'F')
        goto fail;
    if (eh.e_machine != 3 /* EM_386 */)
        goto fail;
    if (eh.e_phoff == 0 || eh.e_phnum == 0)
        goto fail;

    // Load program headers
    Elf32_Phdr* ph = kmalloc(eh.e_phnum * sizeof(Elf32_Phdr));
    if (!ph)
        goto fail;

    vfs_llseek(file, eh.e_phoff, 0);
    vfs_read(file, (uint8_t*)ph, eh.e_phnum * sizeof(Elf32_Phdr), 0);

    proc_t* p = get_proc_from_thread(thread);

    vm_space_t* new_vmspace = vm_space_create();
    if (IS_ERR(new_vmspace)) {
        kfree(ph);
        vfs_close(file);
        return -ENOMEM;
    }

    vm_space_activate(new_vmspace);
    vm_space_destroy(p->vmspace);
    p->vmspace = new_vmspace;

    for (int i = 0; i < eh.e_phnum; i++) {
        if (ph[i].p_type != 1 /* PT_LOAD */)
            continue;

        // Allocate memory in the process's VM space
        vm_map(p->vmspace, &ph[i].p_vaddr, ph[i].p_memsz,
               VM_PROT_READ | VM_PROT_USER | VM_PROT_WRITE, VM_REG_F_PRIVATE, obj, ph[i].p_offset,
               VM_MAP_F_FIXED);
    }

    thread->trapframe->eip = eh.e_entry; // Set entry point for the new executable
    kfree(ph);
    vm_object_dec_ref(obj);
    vfs_close(file);

    return 0;

fail:
    vm_object_dec_ref(obj);
    vfs_close(file);
    return -1;
}
