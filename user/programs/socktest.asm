; Simple socket test - creates socket and listens
BITS 32

SYSCALL_EXIT    equ 0
SYSCALL_PRINT   equ 1
SYSCALL_SOCKET  equ 6
SYSCALL_LISTEN  equ 8
SYSCALL_CLOSE   equ 3

SOCK_TYPE_STREAM equ 1

section .data
    msg_start db "Creating stream socket...", 10, 0
    msg_fd db "Socket FD: ", 0
    msg_listen db "Listening with backlog 5...", 10, 0
    msg_success db "Success! Socket is listening.", 10, 0
    msg_error db "Error occurred!", 10, 0
    newline db 10, 0

section .text
global _start

_start:
    ; Print start message
    mov eax, SYSCALL_PRINT
    mov ebx, msg_start
    int 0x80
    
    ; Create socket
    mov eax, SYSCALL_SOCKET
    mov ebx, SOCK_TYPE_STREAM
    int 0x80
    
    ; Check for error
    cmp eax, 0
    jl .error
    
    ; Save socket FD
    push eax
    
    ; Print FD message  
    mov eax, SYSCALL_PRINT
    mov ebx, msg_fd
    int 0x80
    
    ; TODO: Print the actual FD number (would need itoa)
    
    ; Print listen message
    mov eax, SYSCALL_PRINT
    mov ebx, msg_listen
    int 0x80
    
    ; Listen on socket
    pop ebx                 ; socket FD
    push ebx                ; save it again
    mov eax, SYSCALL_LISTEN
    mov ecx, 5              ; backlog
    int 0x80
    
    ; Check for error
    cmp eax, 0
    jl .error
    
    ; Print success
    mov eax, SYSCALL_PRINT
    mov ebx, msg_success
    int 0x80
    
    ; Close socket
    pop ebx
    mov eax, SYSCALL_CLOSE
    int 0x80
    
    ; Exit successfully
    mov eax, SYSCALL_EXIT
    xor ebx, ebx
    int 0x80

.error:
    mov eax, SYSCALL_PRINT
    mov ebx, msg_error
    int 0x80
    
    mov eax, SYSCALL_EXIT
    mov ebx, 1
    int 0x80
