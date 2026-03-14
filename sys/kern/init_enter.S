#include "machine/asmmacros.h"
#include "machine/genassym.h"

.equ LOAD_ADDR, 0x1000000  # Load the init program at this address

# Uses the execve syscall to start the init process.
ENTRY(load_init)
    # Load the address of the init program into eax
    movl    $(init_name-load_init+LOAD_ADDR), %ebx

    # Set up the arguments for execve
    xorl    %ecx, %ecx      # argv = NULL
    xorl    %edx, %edx      # envp = NULL

    # Call execve syscall (syscall number 11)
    movl    $59, %eax       # syscall number for execve
    int     $0x80           # make the syscall

    # If execve returns, it means there was an error.
    movl    $1, %eax        # syscall number for exit
    xorl    %ebx, %ebx      # exit code 0
    int     $0x80           # make the syscall

init_name:
    .asciz "/sbin/init.elf"
END(load_init)
