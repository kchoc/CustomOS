#include "kernel/process/elf.h"
#include "kernel/memory/vmspace.h"
#include "kernel/process/process.h"
#include "kernel/memory/kmalloc.h"
#include "kernel/filesystem/fs.h"
#include "kernel/terminal.h"
#include "types/string.h"

proc_t *create_process_from_elf(const char *filename) {
  	proc_t* p = create_process(strchr(filename, '/') ? strrchr(filename, '/') + 1 : filename);
    vm_space_switch(p->vmspace);

    fat16_file_t* file = fat16_open_file(filename);
    if (!file) {
        printf("ELF loader: failed to open %s\n", filename);
        free_process(p);
        return NULL;
    }

    Elf32_Ehdr eh;
    fat16_file_read(file, (uint8_t *)&eh, sizeof(eh)); // Read ELF header

    /* Basic ELF validation */
    if (eh.e_ident[0] != 0x7F || eh.e_ident[1] != 'E' ||
        eh.e_ident[2] != 'L'  || eh.e_ident[3] != 'F') {
        printf("ELF loader: invalid magic for %s\n", filename);
        goto fail;
    }
    if (eh.e_machine != 3 /* EM_386 */) {
        printf("ELF loader: unexpected machine (not i386)\n");
        goto fail;
    }
    if (eh.e_phoff == 0 || eh.e_phnum == 0) {
        printf("ELF loader: no program headers\n");
        goto fail;
    }

    // Load program headers
    Elf32_Phdr* ph = kmalloc(eh.e_phnum * sizeof(Elf32_Phdr));
    if (!ph) {
        printf("ELF loader: out of memory\n");
        goto fail;
    }

    fat16_file_seek(file, eh.e_phoff);
    fat16_file_read(file, (uint8_t *)ph, eh.e_phnum * sizeof(Elf32_Phdr));

    for (int i = 0; i < eh.e_phnum; i++) {
        if (ph[i].p_type != 1 /* PT_LOAD */) continue;

        fat16_file_seek(file, ph[i].p_offset);
        fat16_file_read(file, (uint8_t *)ph[i].p_vaddr, ph[i].p_filesz);

        // Zero out the rest if memsz > filesz
        if (ph[i].p_memsz > ph[i].p_filesz) {
            memset((uint8_t *)ph[i].p_vaddr + ph[i].p_filesz, 0, ph[i].p_memsz - ph[i].p_filesz);
        }
    }

    kfree(ph);
    fat16_file_close(file);

    // Entry point
    void (*entry)(void) = (void (*)(void))eh.e_entry;

    // Create main thread
    thread_t* t = create_task(entry, p, 1, NULL);
    p->main_thread = t;

    return p;

    fail:
    fat16_file_close(file);
    free_process(p);
    return NULL;
}
