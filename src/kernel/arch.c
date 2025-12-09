#include "kernel/memory/kmalloc.h"
#include "kernel/memory/vm.h"
#include "kernel/memory/mm.h"
#include "kernel/memory/vmspace.h"
#include "kernel/memory/layout.h"

#include "kernel/descriptors/gdt.h"
#include "kernel/descriptors/idt.h"
#include "kernel/descriptors/rsd.h"
#include "kernel/descriptors/bda.h"

#include "kernel/process/cpu.h"
#include "kernel/process/elf.h"
#include "kernel/terminal.h"
#include "kernel/syscalls/syscalls.h"
#include "kernel/process/process.h"
#include "kernel/process/ap_start.h"

#include "kernel/drivers/port_io.h"
#include "kernel/drivers/pci.h"
#include "kernel/drivers/vbe.h"

#include "kernel/filesystem/vfs.h"
#include "kernel/filesystem/fat16.h"
#include "kernel/filesystem/sockfs.h"
#include "kernel/wm/window.h"

void init() {
    terminal_init();
    printf("Terminal: OK\n");

    kmalloc_init((char*)KMALLOC_START, KMALLOC_SIZE);
    printf("kmalloc: OK\n");

    pmm_init();
    printf("PMM: OK\n");

    vmm_init();
    printf("VMM: OK\n");

    vm_space_init();
    printf("VMSpace: OK\n");

    load_ebda();
    printf("BDA/EBDA: OK\n");

    gdt_init_percpu(0, 0);
    printf("GDT: OK\n");

    rsdt_init();
    printf("RSD: OK\n");

    syscalls_init();
    printf("Syscalls: OK\n");
    
    vbe_init();
    
    wm_init();

    idt_init();
    printf("IDT: OK\n");

    // Enable interrupts
    asm volatile("sti");
    printf("Interrupts: ENABLED\n");

    tasking_init();
    printf("Tasking: OK\n");

    init_cpus();
    start_all_aps();

    pci_discover_devices();

    // Initialize and mount root filesystem
    printf("VFS: %s\n", vfs_mount_drive("QEMU HARDDISK", "/", &fat16_fs_type) == 0 ? "OK" : "FAILED");

    // Initialize sockfs
    printf("Sockfs: %s\n", sockfs_init() == 0 ? "OK" : "FAILED");

    // Map VGA text mode buffer for terminal (still needed for text mode)
    void* vga_text = (void*)0xC03FF000;
    vmm_map(vga_text, 0xB8000, 0x1000, 
            VM_PROT_READWRITE | VM_PROT_NOCACHE, VM_MAP_PHYS | VM_MAP_FORCE);

    // Clear all pages that are not used by the kernel (<0xC0000000)
    vmm_unmap_tables(0, 768); // Unmap first 3GB (768 * 4MB pages)

    create_process_from_elf("terminal.elf");
}
