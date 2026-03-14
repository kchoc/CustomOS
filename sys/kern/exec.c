#include "exec.h"
#include "elf.h"
#include "process.h"
#include "terminal.h"

#include <sys/pcpu.h>

int execve(const char* path, char* const argv[], char* const envp[]) {
    // For now, we only support execve for the current process and ignore envp
    thread_t* thread = PCPU_GET(current_thread);
    if (!thread) return -1;

    // Load the new executable into the process's VM space
    return load_elf(path, thread);
}

