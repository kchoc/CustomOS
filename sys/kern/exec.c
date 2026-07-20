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

#define MAX_ENV_VARS 128

int arg_copy(char* dest[], char* const src[])
{
    int count = 0;
    while (src && src[count] && count < MAX_ENV_VARS) {
        dest[count] = strdup(src[count]);
        count++;
    }
    dest[count] = NULL; // Null-terminate the array
    return count;
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

    // Because when the ELF is loaded the current process's stack is replaced, we need to copy
    // arguments and environment variables into kernel space before loading the ELF. This way we can
    // set up the new stack for the process after loading the ELF.
    char* argv_copy[MAX_ENV_VARS + 1];
    char* envp_copy[MAX_ENV_VARS + 1];
    arg_copy(argv_copy, argv);
    arg_copy(envp_copy, envp);

    if (strcmp(extension, ".elf") == 0) {
        if (load_elf(path, thread)) {
            vfs_close(file);
            return -ENOEXEC;
        }

        vfs_close(file);
        setup_exec_stack(thread, argv_copy, envp_copy);
        return 0;
    }
    vfs_close(file);
    return -ENOEXEC;
}

void setup_exec_stack(thread_t* thread, char* const argv[], char* const envp[])
{
    char*     argv_copy[MAX_ENV_VARS + 1];
    char*     envp_copy[MAX_ENV_VARS + 1];
    uintptr_t stack_top    = 0xC0000000;
    uintptr_t stack_bottom = stack_top - PAGE_SIZE;

    vm_map_anon(get_proc_from_thread(thread)->vmspace, &stack_bottom, PAGE_SIZE,
                VM_PROT_READ | VM_PROT_USER | VM_PROT_WRITE, VM_REG_F_PRIVATE, VM_MAP_F_FIXED);

    // ps_strings at very top
    stack_top -= sizeof(ps_strings_t);
    ps_strings_t* ps_strings = (ps_strings_t*)stack_top;

    // Push argument strings onto stack
    int argc = 0;
    while (argv && argv[argc] && argc < MAX_ENV_VARS) {
        size_t len = strlen(argv[argc]) + 1;
        stack_top -= len;
        memcpy((void*)stack_top, argv[argc], len);
        argv_copy[argc++] = (char*)stack_top;
    }
    argv_copy[argc] = NULL;

    // Push env strings onto stack
    int envc = 0;
    while (envp && envp[envc] && envc < MAX_ENV_VARS) {
        size_t len = strlen(envp[envc]) + 1;
        stack_top -= len;
        memcpy((void*)stack_top, envp[envc], len);
        envp_copy[envc++] = (char*)stack_top;
    }
    envp_copy[envc] = NULL;

    // Align stack to 4 bytes before writing pointer arrays
    stack_top &= ~3;

    // Push envp[] pointer array (null-terminated)
    stack_top -= (envc + 1) * sizeof(char*);
    uintptr_t envp_array = stack_top;
    for (int i = 0; i <= envc; i++) // includes NULL terminator
        ((char**)envp_array)[i] = envp_copy[i];

    // Push argv[] pointer array (null-terminated)
    stack_top -= (argc + 1) * sizeof(char*);
    uintptr_t argv_array = stack_top;
    for (int i = 0; i <= argc; i++) // includes NULL terminator
        ((char**)argv_array)[i] = argv_copy[i];

    stack_top -= sizeof(int);
    *(int*)stack_top = argc;

    // Fill ps_strings
    ps_strings->ps_nargvstr = argc;
    ps_strings->ps_nenvstr  = envc;
    ps_strings->ps_argvstr  = (char**)argv_array;
    ps_strings->ps_envstr   = (char**)envp_array;

    thread->trapframe->user_esp = stack_top;
}
