_start:
    xorl %ebp, %ebp
    movl (%esp), %eax          # argc
    leal 4(%esp), %ecx         # argv
    leal 8(%esp,%eax,4), %edx  # envp
    pushl %edx
    pushl %ecx
    pushl %eax
    call main
    pushl %eax
    movl $1, %eax
    pushl %eax
    int $0x80
