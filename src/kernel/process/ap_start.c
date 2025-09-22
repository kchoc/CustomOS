#include "kernel/process/ap_start.h"
#include "kernel/process/lapic.h"
#include "kernel/process/process.h"
#include "kernel/process/cpu.h"
#include "kernel/memory/vm.h"
#include "kernel/memory/kmalloc.h"
#include "kernel/descriptors/idt.h"
#include "kernel/time/pit.h"
#include "kernel/terminal.h"

/* trampoline placement: choose a page-aligned low physical address */
#define TRAMPOLINE_PHYS 0x00009000  /* 4KB aligned physical address for trampoline */
#define STACK_SIZE  8192        /* 8KB stack for AP */

/* called from trampoline (extern symbol) */
void ap_entry(void) {
    uint32_t apic = get_local_apic_id();
    int cpu_idx = map_apicid_to_index(apic);

    if (cpu_idx < 0) {
        // unknown APIC id; spin
        for(;;) asm volatile("hlt");
    }

    cpu_t* cpu = &cpus[cpu_idx];
    cpu->started = 1;

    thread_t* idle = create_task(idle_task, idle_process, 0, cpu);
    cpu->current_thread = idle;
    apic_cpu_init();
    load_idt();
    asm volatile("sti"); // enable interrupts
    apic_timer_init(10000000);
    schedule();
}

/* Copy the ap_entry address for the trampoline to a known location */
void deploy_trampoline(void) {
    // shared location in low memory (must be identity-mapped)
    uint32_t *ap_entry_ptr = (uint32_t*)0x7000;
    *ap_entry_ptr = (uint32_t)ap_entry;
    uint32_t *ap_cr3_ptr = (uint32_t*)0x7004;
    *ap_cr3_ptr = (uint32_t)get_current_page_directory_phys();
}

/* Start a single AP by APIC id */
int start_ap(uint32_t apic_id) {
    /* Optional: clear a flag in cpus[] so BSP can wait for AP to set it */
    int cpu_idx = map_apicid_to_index(apic_id);
    if (cpu_idx < 0) return -1;
    cpus[cpu_idx].started = 0;

    uint32_t *ap_stack_ptr = (uint32_t*)0x700C;
    *ap_stack_ptr = (uint32_t)kmalloc(STACK_SIZE) + STACK_SIZE;

    /* INIT */
    send_init_ipi(apic_id);
    delay_ms(20);

    /* SIPI(s) pointing at TRAMPOLINE_PADDR >> 12 */
    // two SIPIs, 200us apart
    send_startup_ipi(apic_id, TRAMPOLINE_PHYS);

    /* wait for AP to signal started (timeout) */
    uint16_t* ap_check_ptr = (uint16_t*)0x7008;
    for (int i = 0; i < 200; ++i) {
        // printf("ap_check_ptr=%x val=%x\n", ap_check_ptr, *ap_check_ptr);
        if (cpus[cpu_idx].started) return 0;
        delay_ms(10);
    }
    return -1; /* timeout */
}

/* Start all APs given an array of APIC IDs (skip BSP) */
void start_all_aps() {
    lapic_init();
    deploy_trampoline();

    printf("APs: %u/%u(MAX)\n", cpu_count, MAX_CPUS);
    for (size_t i = 0; i < cpu_count; ++i) {
        uint32_t id = cpus[i].apic_id;
        if (id == get_local_apic_id()) continue; /* skip BSP */
        printf("APIC CPU %u: ", id);
        if (start_ap(id) == 0)
            printf("OK\n");
        else
            printf("FAILED\n");
    }
}
