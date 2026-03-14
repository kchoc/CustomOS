#ifndef _I386_CONTEXT_H_
#define _I386_CONTEXT_H_

#include <stdbool.h>
#include <inttypes.h>

typedef struct thread thread_t;

typedef struct context_t {
    uint32_t edi;
    uint32_t esi;
    uint32_t ebx;
    uint32_t ebp;
    uint32_t eip; // Instruction pointer
} context_t;

extern void start_thread(void);
extern void context_switch(context_t** old, context_t* new);

void context_init(thread_t* thread, void (*entry)(void), uint32_t* stack_top, uint32_t user_stack_top);

#endif // _I386_CONTEXT_H_
