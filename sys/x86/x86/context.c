#include <kern/process.h>
#include <machine/context.h>
#include <machine/segment.h>

#include <inttypes.h>

void context_init(thread_t* thread, void (*entry)(void), uintptr_t stack_top,
                  uintptr_t user_stack_top)
{
    unsigned int* stk = (unsigned int*)stack_top;
    *(--stk)          = (uintptr_t)thread_exit; // Return address
    *(--stk)          = user_stack_top;   // Initial user stack pointer (will be loaded into ESP)
    *(--stk)          = (uintptr_t)entry; // Task entry point (will be loaded into EIP)
    *(--stk)          = (uintptr_t)start_thread; // Context switch will start executing here
    *(--stk)          = 0;                       // EBP
    *(--stk)          = 0;                       // EBX
    *(--stk)          = 0;                       // ESI
    *(--stk)          = 0;                       // EDI
    thread->context   = (context_t*)stk; // Context will be loaded from here during context switch
}

void context_fork(thread_t* child, thread_t* parent, uintptr_t stack_top)
{
    // Copy the trapframe from the parent thread to the child thread, but modify it so that when the
    // child thread is scheduled to run, it will start executing at start_fork instead of the
    // original entry point. start_fork will then call the original entry point after setting up the
    // correct return value for fork (0 in the child).
    uint32_t* stk = (uint32_t*)stack_top;
    stk -= sizeof(trapframe_t) / sizeof(uintptr_t); // Make space for trapframe
    trapframe_t* child_tf  = (trapframe_t*)stk;
    trapframe_t* parent_tf = parent->trapframe;
    *child_tf              = *parent_tf; // Copy entire trapframe
    child_tf->eax          = 0;          // Set return value of fork to 0 in child
    *(--stk) =
        (uintptr_t)start_fork; // Task entry point for forked thread (will be loaded into EIP)
    *(--stk)       = 0;        // EBP
    *(--stk)       = 0;        // EBX
    *(--stk)       = 0;        // ESI
    *(--stk)       = 0;        // EDI
    child->context = (context_t*)stk; // Context will be loaded from here during context switch
}
