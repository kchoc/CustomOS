#ifndef SYSCALLS_H
#define SYSCALLS_H

#include <stdint.h>

#define SYSCALL_COUNT 256

extern void* g_syscalls[];

void syscalls_init();
int syscall(uint32_t syscall_id, int arg_count, ...);

int syscall_exit();
int syscall_print(const char* str);

#endif //SYSCALLS_H