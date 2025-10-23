global _start

section .text

_start:
    mov eax, 1          ; syscall_print
    mov ebx, message
    int 0x80            ; trigger syscall

    int 0x1

    ; Catch halt if exit fails
hang:
    jmp hang

message:
    db "Hello from userland!", 0
