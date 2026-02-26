// #include "initcpu.h"
// #include <machine/pcpu.h>

// #include <inttypes.h>
// #include <stdint.h>
// #include <stdbool.h>

// bool trampoline_deployed = false;

// /* trampoline placement: choose a page-aligned low physical address */
// #define TRAMPOLINE_PHYS 0x00009000  /* 4KB aligned physical address for trampoline */
// #define STACK_SIZE  8192        /* 8KB stack for AP */

// /* called from trampoline (extern symbol) */
// void ap_entry(void) __attribute__((noinline, used));
// void ap_entry(void) {
//     pcpu_t* cpu = get_current_pcpu();

//     thread_t* idle = create_kernel_thread(idle_task, idle_process, 0, cpu);
//     cpu->current_thread = idle;


//     load_idt();

//     apic_cpu_init();
//     vm_space_activate(idle_process->vmspace);
    
//     gdt_init_percpu(cpu_idx, (uint32_t)(idle->kstack + idle->kstack_size));

//     asm volatile("sti"); // enable interrupts
//     apic_timer_init(10000000);

//     // hault till schedule
//     for (;;) asm volatile("hlt");
// }

// /* Copy the ap_entry address for the trampoline to a known location */
// void deploy_trampoline(void) {
//     vm_map(NULL, 0x7000, TRAMPOLINE_PHYS, PAGE_SIZE, VM_PROT_READ | VM_PROT_WRITE, VM_MAP_PHYS | VM_MAP_FORCE);
    
//     // shared location in low memory (must be identity-mapped)
//     uint32_t *ap_entry_ptr = (uint32_t*)0x7000;

//     // For some reason this returns the link address, not the actual address?
//     printf("Trampoline at %x, ap_entry at %x\n", TRAMPOLINE_PHYS, (uintptr_t)(void*)ap_entry);
//     *ap_entry_ptr = (uint32_t)ap_entry;
//     uint32_t *ap_cr3_ptr = (uint32_t*)0x7004;
//     asm volatile("mov %%cr3, %0" : "=r"(*ap_cr3_ptr) :: "memory");

//     vm_unmap(NULL, 0x7000, PAGE_SIZE);
//     trampoline_deployed = true;
// }

// int init_primary_cpu(cpu_t* cpu) {
// 	// Initialize primary CPU (BSP)
// 	cpu->started = 1;
// 	return 0;
// }

// int init_secondary_cpu(cpu_t* cpu) {
// 	// Deploy trampoline if not already done
// 	if (!trampoline_deployed) deploy_trampoline();

// 	// Initialize secondary CPU (AP)
// 	cpu->started = 0;
// 	uint32_t apic_id = ((x86_cpu_state_t*)(cpu.arch))->apic_id;

// 	uint32_t *ap_stack_ptr = (uint32_t*)0x700C;
//     *ap_stack_ptr = (uint32_t)kmalloc(STACK_SIZE) + STACK_SIZE;

//     /* INIT */
//     send_init_ipi(apic_id);
//     delay_ms(20);

//     /* SIPI(s) pointing at TRAMPOLINE_PADDR >> 12 */
//     // two SIPIs, 200us apart
//     send_startup_ipi(apic_id, TRAMPOLINE_PHYS);

//     /* wait for AP to signal started (timeout) */
//     uint16_t* ap_check_ptr = (uint16_t*)0x7008;
//     for (int i = 0; i < 200; ++i) {
//         // printf("ap_check_ptr=%x val=%x\n", ap_check_ptr, *ap_check_ptr);
//         if (cpu.started) return 0;
//         delay_ms(10);
//     }
//     return -1; /* timeout */
// }

// /* Start all APs given an array of APIC IDs (skip BSP) */
// void start_all_aps() {
//     lapic_init();

//     // Setup BSP LAPIC and IOAPIC
//     apic_cpu_init();
//     ioapic_init();
//     ioapic_enable_irq(1, 0x21, 0); // Keyboard IRQ

//     // Deploy trampoline code
//     deploy_trampoline();

//     printf("APs: %u/%u(MAX)\n", cpu_count, MAX_CPUS);
//     for (size_t i = 0; i < cpu_count; ++i) {
//         uint32_t id = ((x86_cpu_state_t*)(cpus[i].arch))->apic_id;
//         if (id == get_local_apic_id()) {
//             continue; /* skip BSP */
//         }
//         printf("APIC CPU %u: ", id);
//         if (start_ap(id) == 0)
//             printf("OK\n");
//         else
//             printf("FAILED\n");
//     }
    
//     apic_timer_init(10000000);
// }

