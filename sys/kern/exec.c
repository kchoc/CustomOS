#include "exec.h"
#include "elf.h"
#include "errno.h"
#include "panic.h"
#include "process.h"
#include "terminal.h"

#include <fs/vfs.h>

#include <sys/pcpu.h>

#include <string.h>

int execve(const char* path, char* const argv[], char* const envp[])
{
    // For now, we only support execve for the current process and ignore envp
    thread_t* thread = PCPU_GET(current_thread);
    if (!thread)
        PANIC("execve: No current thread");

    if (!path || path[0] == '\0')
        return -ENOENT;
    const char* extension = strrchr(path, '.');
    if (!extension || strcmp(extension, ".elf") != 0)
        return -ENOEXEC;
    file_t* file = vfs_open(path, 0, 0);
    if (!file)
        return -ENOENT;

    if (strcmp(extension, ".elf") == 0) {
        if (load_elf(path, thread) != 0) {
            vfs_close(file);
            return -ENOEXEC;
        }
        return 0;
    }
    vfs_close(file);
    return -ENOEXEC;
}
