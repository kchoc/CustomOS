#include "kernel/process/lapic.h"
#include "kernel/memory/vm.h"
#include "kernel/time/pit.h"

/* ---------------- LAPIC Access ---------------- */

static inline uint32_t lapic_read(uint32_t reg) {
    volatile uint32_t *addr = (volatile uint32_t*)(LAPIC_BASE + reg);
    return *addr;
}

static inline void lapic_write(uint32_t reg, uint32_t value) {
    volatile uint32_t *addr = (volatile uint32_t*)(LAPIC_BASE + reg);
    *addr = value;
}

/* Read local APIC ID (bits 31:24 of ID register) */
uint32_t get_local_apic_id(void) {
    uint32_t val = lapic_read(LAPIC_ID);
    return (val >> 24) & 0xFF;
}

/* Wait until ICR send is accepted (bit 12 = delivery status) */
static void lapic_wait_for_delivery(void) {
    while (lapic_read(LAPIC_ICR_LOW) & (1 << 12)) {
        asm volatile("pause");
    }
}

/* Send an IPI to a single APIC ID
 * icr_high is destination in high dword (apicid << 24)
 * icr_low contains vector and delivery mode/config
 */
static void lapic_send_ipi(uint32_t apic_id, uint32_t icr_low) {
    /* write high dword (destination) */
    lapic_write(LAPIC_ICR_HIGH, apic_id << 24);
    /* write low dword (vector/delivery mode) */
    lapic_write(LAPIC_ICR_LOW, icr_low);
    lapic_wait_for_delivery();
}

void lapic_init() {
	// Map LAPIC base
    vmm_map((void*)LAPIC_BASE, LAPIC_BASE, PAGE_SIZE, VM_PROT_READWRITE, VM_MAP_PHYS | VM_MAP_ZERO);


	// Enable APIC by setting the spurious interrupt vector register (SVR)
	// Set bit 8 (APIC enabled) and set vector to 0xFF (example)
	lapic_write(LAPIC_SVR, 0x100 | 0xFF);
}

/* ---------------- IPI Sending ---------------- */

/* Send INIT to AP (assert) then deassert according to MP spec.
Summary:
 * 1. Write to ICR high dword with destination APIC ID
 * 2. Write to ICR low dword with INIT delivery mode, assert level, physical mode
 * 3. Wait 10ms (spec says 10ms - 100ms between INIT and SIPI)
 * 4. Write to ICR low dword again with INIT delivery mode, deassert level, physical mode
 * 5. Wait another 10ms
 */
void send_init_ipi(uint32_t apic_id) {
    /* INIT assert */
    lapic_send_ipi(apic_id, ICR_DELIVERY_INIT | ICR_ASSERT_LEVEL | ICR_PHYSICAL);
    /* wait 10ms (spec says 10ms - 100ms between INIT and SIPI) */
    delay_ms(10);
    /* Some hw requires a deassert; do it by sending INIT with deassert flag 0 */
    lapic_send_ipi(apic_id, ICR_DELIVERY_INIT | ICR_PHYSICAL);
    delay_ms(10);
}

/* Send one or two SIPIs to the AP pointing at (trampoline_paddr >> 12) */
void send_startup_ipi(uint32_t apic_id, uint32_t trampoline_paddr) {
    uint32_t vector = (trampoline_paddr >> 12) & 0xFF;
    uint32_t icr = ICR_DELIVERY_SIPI | (vector & 0xFF) | ICR_PHYSICAL;

    // SIPI: vector in low 8 bits | SIPI delivery mode
    lapic_send_ipi(apic_id, icr);
    // wait 200us - 1ms per spec; then send a second SIPI
    delay_ms(1);
    // send a second SIPI to be safe on some hardware
    lapic_send_ipi(apic_id, icr);
}
