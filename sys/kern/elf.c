#include "elf.h"
#include "process.h"
#include "terminal.h"

#include <vm/vm_map.h>
#include <vm/kmalloc.h>

#include <fs/vfs.h>
#include <string.h>
#include <stdint.h>

proc_t *create_process_from_elf(const char *filename) {
    proc_t* p = create_process(strchr(filename, '/') ? strrchr(filename, '/') + 1 : filename);
    if (!p) return NULL;

    file_t* file = vfs_open(filename, 0, 0);
    if (!file) { free_process(p); return NULL; }

    Elf32_Ehdr eh;
    vfs_read(file, (uint8_t *)&eh, sizeof(Elf32_Ehdr), NULL);

    /* Basic ELF validation */
    if (eh.e_ident[0] != 0x7F || eh.e_ident[1] != 'E' || eh.e_ident[2] != 'L'  || eh.e_ident[3] != 'F') goto fail;
    if (eh.e_machine != 3 /* EM_386 */) goto fail;
    if (eh.e_phoff == 0 || eh.e_phnum == 0) goto fail;

    // Load program headers
    Elf32_Phdr* ph = kmalloc(eh.e_phnum * sizeof(Elf32_Phdr));
    if (!ph) goto fail;

    vfs_llseek(file, eh.e_phoff, 0);
    vfs_read(file, (uint8_t *)ph, eh.e_phnum * sizeof(Elf32_Phdr), NULL);

    for (int i = 0; i < eh.e_phnum; i++) {
        if (ph[i].p_type != 1 /* PT_LOAD */) continue;

        // Allocate memory in the process's VM space
        vm_map_anon(p->vmspace, &ph[i].p_vaddr, ph[i].p_memsz, 
                               VM_PROT_READ | VM_PROT_USER, VM_REG_F_PRIVATE);

        // Load file data into the mapped memory
        vfs_llseek(file, ph[i].p_offset, 0);
        vfs_read(file, (uint8_t *)ph[i].p_vaddr, ph[i].p_filesz, NULL);
    }

    kfree(ph);
    vfs_close(file);

    // Create main thread
    thread_t* t = create_user_thread((void (*)(void))eh.e_entry, p, 1, NULL);
    p->main_thread = t;

    return p;

    fail:
    vfs_close(file);
    free_process(p);
    return NULL;
}
