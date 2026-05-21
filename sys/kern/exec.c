#include "exec.h"
#include "elf.h"
#include "errno.h"
#include "panic.h"
#include "process.h"
#include "terminal.h"

#include <fs/vfs.h>

#include <sys/pcpu.h>

#include <vm/vm_map.h>

#include <inttypes.h>
#include <string.h>

void arg_count(char* const* arr, int* count, size_t* total_size)
{
    int    n    = 0;
    size_t size = 0;
    while (arr && arr[n]) {
        size += strlen(arr[n]) + 1; // +1 for null terminator
        n++;
    }
    if (count)
        *count = n;
    if (total_size)
        *total_size = size;
}

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
        if (load_elf(path, thread)) {
            vfs_close(file);
            return -ENOEXEC;
        }
        goto success;
    }
    vfs_close(file);
    return -ENOEXEC;

    uintptr_t stack_bottom;
    uintptr_t stack_top;
success:
    stack_top    = 0xC0000000;
    stack_bottom = stack_top - PAGE_SIZE;
    vm_map_anon(get_proc_from_thread(thread)->vmspace, &stack_bottom, PAGE_SIZE,
                VM_PROT_READ | VM_PROT_USER | VM_PROT_WRITE, VM_REG_F_PRIVATE, VM_MAP_F_FIXED);

    int    argc, envc;
    size_t argv_size, envp_size;
    arg_count(argv, &argc, &argv_size);
    arg_count(envp, &envc, &envp_size);

    uintptr_t envp_strings_start = stack_top - envp_size - sizeof(ps_strings_t);
    uintptr_t argv_strings_start = envp_strings_start - argv_size;
    uintptr_t envp_array_start =
        argv_strings_start - (envc + 1) * sizeof(char*); // +1 for null terminator
    uintptr_t argv_array_start =
        envp_array_start - (argc + 1) * sizeof(char*); // +1 for null terminator

    // Set up the user stack with metadata for the new process (argv, envp, etc.)
    stack_top -= sizeof(ps_strings_t);
    ps_strings_t* ps_strings = (ps_strings_t*)stack_top;
    ps_strings->ps_nargvstr  = argc;
    ps_strings->ps_nenvstr   = envc;
    ps_strings->ps_argvstr   = (char**)argv_array_start;
    ps_strings->ps_envstr    = (char**)envp_array_start;

    stack_top = argv_array_start - sizeof(int);

    for (int i = 0; i < argc; i++) {
        size_t arg_len = strlen(argv[i]) + 1;
        argv_array_start += sizeof(char*);
        *(char**)argv_array_start = argv_strings_start;
        memcpy((void*)argv_strings_start, argv[i], arg_len);
        argv_strings_start += arg_len;
    }

    for (int i = 0; i < envc; i++) {
        size_t env_len = strlen(envp[i]) + 1;
        envp_array_start += sizeof(char*);
        *(char**)envp_array_start = envp_strings_start;
        memcpy((void*)envp_strings_start, envp[i], env_len);
        envp_strings_start += env_len;
    }

    // Set argc at the top of the stack for the new process
    *(int*)stack_top = argc;

    thread->trapframe->user_esp = stack_top;

    vfs_close(file);
    return 0;
}
