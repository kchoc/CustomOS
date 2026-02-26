#ifndef x86_I386_CONTEXT_H
#define x86_I386_CONTEXT_H

#include <inttypes.h>

typedef struct i386_context {
	uint32_t edi, esi, ebp, esp;
	uint32_t ebx, edx, ecx, eax;
	uint32_t eip, eflags;
} i386_context_t;

void i386_context_switch(i386_context_t* old_ctx, i386_context_t* new_ctx);

#endif // x86_I386_CONTEXT_H
