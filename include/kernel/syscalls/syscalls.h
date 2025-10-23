#ifndef SYSCALLS_H
#define SYSCALLS_H

#include "kernel/types.h"
#include "types/common.h"

#define SYSCALL_COUNT 256

#define SYSCALL_EXIT   0
#define SYSCALL_PRINT  1

// Macro to define syscall function prototypes so that syscalls using < 5 args can be defined easily
#define SYSCALL1 uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t arg5
#define SYSCALL2 uint32_t arg3, uint32_t arg4, uint32_t arg5
#define SYSCALL3 uint32_t arg4, uint32_t arg5
#define SYSCALL4 uint32_t arg5

extern void* g_syscalls[];

typedef int (*syscall_fn_t)(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);

void syscalls_init();
int syscall(uint32_t syscall_id, int arg_count, ...);

int syscall_exit(registers_t* regs);
int syscall_print(const char* str, SYSCALL1);

#endif //SYSCALLS_H