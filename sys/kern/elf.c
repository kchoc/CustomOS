#include "elf.h"
#include "process.h"
#include "terminal.h"

#include <sys/pcpu.h>

#include <fs/vfs.h>

#include <vm/kmalloc.h>
#include <vm/vm_map.h>

#include <stdint.h>

// REQUIRES that the thread be the current thread
int load_elf(const char* filepath, thread_t* thread)
{
    if (!filepath || !thread || thread != PCPU_GET(current_thread))
        return -1;

    file_t* file = vfs_open(filepath, 0, 0x1);
    if (!file)
        return -1;

    Elf32_Ehdr eh;
    int res = vfs_read(file, (uint8_t*)&eh, sizeof(Elf32_Ehdr), 0);

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

    vm_space_clean(thread->proc->vmspace); // Clean existing VM space (unmap old executable)


    for (int i = 0; i < eh.e_phnum; i++) {
        if (ph[i].p_type != 1 /* PT_LOAD */)
            continue;

        // Allocate memory in the process's VM space
        vm_map_anon(thread->proc->vmspace, &ph[i].p_vaddr, ph[i].p_memsz,
                    VM_PROT_READ | VM_PROT_USER | VM_PROT_WRITE, VM_REG_F_PRIVATE, VM_MAP_F_FIXED);

        // Load file data into the mapped memory
        vfs_llseek(file, ph[i].p_offset, 0);
        vfs_read(file, (uint8_t*)ph[i].p_vaddr, ph[i].p_filesz, 0);
    }

    thread->trapframe->eip      = eh.e_entry; // Set entry point for the new executable
    uint32_t stack_bottom = 0xC0000000 - PAGE_SIZE;
    vm_map_anon(thread->proc->vmspace, &stack_bottom, PAGE_SIZE, VM_PROT_READ | VM_PROT_USER | VM_PROT_WRITE,
                VM_REG_F_PRIVATE, VM_MAP_F_FIXED); // Map user stack 

    thread->trapframe->user_esp = stack_bottom + PAGE_SIZE; 
    kfree(ph);
    vfs_close(file);

    return 0;

fail:
    vfs_close(file);
    return -1;
}
