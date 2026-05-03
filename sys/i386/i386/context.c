#include <kern/process.h>
#include <machine/context.h>
#include <machine/segment.h>

#include <inttypes.h>

void context_init(thread_t* thread, void (*entry)(void), uint32_t* stack_top,
                  uint32_t user_stack_top)
{
    *(--stack_top) = (uint32_t)thread_exit;  // Return address
    *(--stack_top) = user_stack_top;         // Initial user stack pointer (will be loaded into ESP)
    *(--stack_top) = (uint32_t)entry;        // Task entry point (will be loaded into EIP)
    *(--stack_top) = (uint32_t)start_thread; // Context switch will start executing here
    *(--stack_top) = 0;                      // EBP
    *(--stack_top) = 0;                      // EBX
    *(--stack_top) = 0;                      // ESI
    *(--stack_top) = 0;                      // EDI
    thread->context =
        (context_t*)stack_top; // Context will be loaded from here during context switch
    return;
}

void context_fork(thread_t* child, thread_t* parent, uint32_t* stack_top){
    // Copy the trapframe from the parent thread to the child thread, but modify it so that when the child thread is scheduled to run, it will start executing at start_fork instead of the original entry point. start_fork will then call the original entry point after setting up the correct return value for fork (0 in the child).
    stack_top -= sizeof(trapframe_t) / sizeof(uint32_t); // Make space for trapframe
    trapframe_t* child_tf = (trapframe_t*)stack_top;
    trapframe_t* parent_tf = parent->trapframe;
    *child_tf = *parent_tf; // Copy entire trapframe 
    child_tf->eax = 0; // Set return value of fork to 0 in child
    *(--stack_top) = (uint32_t)start_fork;   // Task entry point for forked thread (will be loaded into EIP)
    *(--stack_top) = 0;                      // EBP
    *(--stack_top) = 0;                      // EBX
    *(--stack_top) = 0;                      // ESI
    *(--stack_top) = 0;                      // EDI
    child->context = (context_t*)stack_top; // Context will be loaded from here during context switch
    
    delay(1000);
} 
