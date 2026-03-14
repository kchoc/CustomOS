global _start

section .text

_start:
    mov eax, 100         ; syscall_print
    mov ebx, message
    int 0x80            ; trigger syscall

    mov eax, 1           ; syscall_exit
    int 0x80            ; syscall_exit

    ; Catch halt if exit fails
hang:
    jmp hang

message:
    ; message with newline and null terminator
    db "Hello from userland!", 0x0A, 0
