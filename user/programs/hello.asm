bits 32
org 0x00001000

section .text
global _start

_start:
    mov eax, 1          ; syscall_print
    mov ebx, message
    int 0x80            ; trigger syscall

    mov eax, 0          ; syscall_exit
    int 0x80            ; trigger syscall

message:
    db "Hello from userland!", 0
