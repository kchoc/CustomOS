#include "kernel/process/lapic.h"
#include "kernel/process/cpu.h"
#include "kernel/terminal.h"
#include "types/string.h"

/* trampoline placement: choose a page-aligned low physical address */
#define TRAMPOLINE_PADDR 0x00008000U   /* 32KiB, page aligned (vector = 0x8) */

/* linker-provided symbols for the binary data */
extern unsigned char _binary_trampoline_bin_start[];
extern unsigned char _binary_trampoline_bin_end[];

static inline uint32_t trampoline_size(void) {
    return (uint32_t)(_binary_trampoline_bin_end - _binary_trampoline_bin_start);
}

/* Copy trampoline to physical low memory (must be writable/identity mapped) */
void deploy_trampoline(void) {
    uint32_t size = trampoline_size();
    void *dst = (void*)TRAMPOLINE_PADDR;

    /* Ensure you can write to TRAMPOLINE_PADDR. If you run with paging and no identity mapping,
       you must temporary map that physical page into the kernel virtual map before copying. */
    memcpy(dst, _binary_trampoline_bin_start, size);
    /* memory barrier to ensure writes visible before SIPI */
    asm volatile("mfence" ::: "memory");
}

/* Start a single AP by APIC id */
int start_ap(uint32_t apic_id) {
    deploy_trampoline();
    /* Optional: clear a flag in cpus[] so BSP can wait for AP to set it */
    int cpu_idx = map_apicid_to_index(apic_id);
    if (cpu_idx < 0) return -1;
    cpus[cpu_idx].started = 0;

    /* INIT */
    send_init_ipi(apic_id);
    delay_ms(10);

    /* SIPI(s) pointing at TRAMPOLINE_PADDR */
    send_startup_ipi(apic_id, TRAMPOLINE_PADDR);

    /* wait for AP to signal started (timeout) */
    for (int i = 0; i < 200; ++i) {
        if (cpus[cpu_idx].started) return 0;
        delay_ms(10);
    }
    return -1; /* timeout */
}

/* Start all APs given an array of APIC IDs (skip BSP) */
void start_all_aps(uint32_t *apic_ids, uint32_t count) {
    for (size_t i = 0; i < count; ++i) {
        uint32_t id = apic_ids[i];
        if (id == get_local_apic_id()) continue; /* skip BSP */
        if (start_ap(id) == 0) {
            // success
        } else {
            // handle failure (log, retry, continue)
        }
    }
}

/* called from trampoline (extern symbol) */
void ap_entry(void) {
    uint32_t apic = get_local_apic_id();
    int cpu_idx = map_apicid_to_index(apic);
    if (cpu_idx < 0) {
        // unknown APIC id; spin
        for(;;) asm volatile("hlt");
    }

    printf("AP CPU %d (APIC ID %u) starting up...\n", cpu_idx, apic);
}
