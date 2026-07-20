#ifndef _I386_CONTEXT_H_
#define _I386_CONTEXT_H_

#include <inttypes.h>
#include <stdbool.h>

typedef struct thread thread_t;

typedef struct context_t {
    unsigned long _di;
    unsigned long _si;
    unsigned long _bx;
    unsigned long _bp;
    unsigned long _ip;
} context_t;

extern void start_thread(void);
extern void start_fork(void);
extern void context_switch(context_t** old, context_t* new);

void context_init(thread_t* thread, void (*entry)(void), uintptr_t stack_top,
                  uintptr_t user_stack_top);
void context_fork(thread_t* child, thread_t* parent, uintptr_t stack_top);

#endif // _I386_CONTEXT_H_
