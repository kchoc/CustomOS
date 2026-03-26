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
}

