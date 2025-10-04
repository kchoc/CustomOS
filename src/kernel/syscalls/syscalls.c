#include "kernel/syscalls/syscalls.h"
#include "kernel/process/process.h"
#include "kernel/terminal.h"
#include "types/string.h"
#include "types/common.h"

#include <stdarg.h>

void* g_syscalls[SYSCALL_COUNT];

void syscalls_init() {
	memset((uint8_t*)g_syscalls, 0, sizeof(void*) * SYSCALL_COUNT);

	g_syscalls[0] = syscall_exit;
    g_syscalls[1] = syscall_print;
}

int syscall_exit() {
	printf("Exiting!\n");
    delay(300);
    thread_exit();
    return 0; // Success
}

int syscall_print(const char* str) {
    if (!str) return -1; // Invalid string
    terminal_print(str);
    return 0; // Success
}


int syscall(uint32_t syscall_id, int arg_count, ...) {
    uint32_t args[5] = {0};

    va_list ap;
    va_start(ap, arg_count);
    for (int i = 0; i < arg_count && i < 5; ++i) {
        args[i] = va_arg(ap, uint32_t);
    }
    va_end(ap);

    int ret;
    asm volatile (
        "int $0x80"
        : "=a"(ret)                     // output: return value in eax
        : "a"(syscall_id),              // input: syscall number in eax
          "b"(args[0]),                 // arg1 in ebx
          "c"(args[1]),                 // arg2 in ecx
          "d"(args[2]),                 // arg3 in edx
          "S"(args[3]),                 // arg4 in esi
          "D"(args[4])                  // arg5 in edi
        : "memory"
    );

    return ret;
}
