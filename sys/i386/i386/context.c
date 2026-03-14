#include <machine/context.h>
#include <machine/segment.h>
#include <kern/process.h>

#include <inttypes.h>

void context_init(thread_t* thread, void (*entry)(void), uint32_t* stack_top, uint32_t user_stack_top) {
    *(--stack_top) = (uint32_t)thread_exit; // Return address
    *(--stack_top) = user_stack_top; // Initial user stack pointer (will be loaded into ESP)
    *(--stack_top) = (uint32_t)entry; // Task entry point (will be loaded into EIP)
    *(--stack_top) = (uint32_t)start_thread; // Context switch will start executing here
    *(--stack_top) = 0; // EBP
    *(--stack_top) = 0; // EBX
    *(--stack_top) = 0; // ESI
    *(--stack_top) = 0; // EDI
    thread->context = (context_t*)stack_top; // Context will be loaded from here during context switch
    return;
    thread->trapframe->eip = (uint32_t)entry;
    thread->trapframe->esp_dummy = (uint32_t)stack_top; // This will be loaded into ESP during context switch
    if (1) {
        thread->trapframe->user_esp = 0; // Not used for kernel threads
        thread->trapframe->ss = GSEL(GDATA_SEL, SEL_KPL); // Kernel data segment
        thread->trapframe->cs = GSEL(GCODE_SEL, SEL_KPL); // Kernel code segment
        thread->trapframe->ds = GSEL(GDATA_SEL, SEL_KPL); // Kernel data segment
        thread->trapframe->es = GSEL(GDATA_SEL, SEL_KPL); // Kernel data segment
        thread->trapframe->fs = 0x0;
        thread->trapframe->gs = 0x0;
        thread->trapframe->eflags = PSL_KERNEL; // Kernel mode flags (interrupts disabled)
    } else {
        thread->trapframe->user_esp = (uint32_t)stack_top; // Initial user stack pointer
        thread->trapframe->ss = GSEL(GUDATA_SEL, SEL_UPL);
        thread->trapframe->cs = GSEL(GUCODE_SEL, SEL_UPL);
        thread->trapframe->ds = GSEL(GUDATA_SEL, SEL_UPL);
        thread->trapframe->es = GSEL(GUDATA_SEL, SEL_UPL);
        thread->trapframe->fs = GSEL(GUDATA_SEL, SEL_UPL);
        thread->trapframe->gs = GSEL(GUDATA_SEL, SEL_UPL);
        thread->trapframe->eflags = PSL_USER; // User mode flags (interrupts enabled)
    }
}

