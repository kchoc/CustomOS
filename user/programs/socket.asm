; Socket test program - demonstrates Unix domain socket IPC
; Build: nasm -f elf32 socket_demo.asm -o socket_demo.o
; Link: ld -m elf_i386 -T userlink.ld socket_demo.o -o socket_demo

BITS 32

; Syscall numbers
SYSCALL_EXIT    equ 0
SYSCALL_PRINT   equ 1
SYSCALL_OPEN    equ 2
SYSCALL_CLOSE   equ 3
SYSCALL_READ    equ 4
SYSCALL_WRITE   equ 5
SYSCALL_SOCKET  equ 6
SYSCALL_CONNECT equ 7
SYSCALL_LISTEN  equ 8
SYSCALL_ACCEPT  equ 9
SYSCALL_SEND    equ 10
SYSCALL_RECV    equ 11
SYSCALL_UNLINK  equ 12

; Socket types
SOCK_TYPE_STREAM equ 1

section .data
    server_path db "/sock/test_server", 0
    msg_creating db "Creating server socket...", 10, 0
    msg_listening db "Server listening on /sock/test_server", 10, 0
    msg_waiting db "Waiting for connections...", 10, 0
    msg_accepted db "Connection accepted!", 10, 0
    msg_received db "Received: ", 0
    msg_sending db "Sending response...", 10, 0
    newline db 10, 0
    
    client_msg db "Hello from client!", 0
    server_response db "Hello from server!", 0

section .bss
    recv_buffer resb 256
    server_fd resd 1
    client_fd resd 1

section .text
global _start

_start:
    ; Fork would go here in a real OS
    ; For demo, we'll do server and client sequentially
    
    ; ===== SERVER SETUP =====
    call print_str
    dd msg_creating
    
    ; Create server socket
    mov eax, SYSCALL_SOCKET
    mov ebx, SOCK_TYPE_STREAM
    int 0x80
    cmp eax, 0
    jl .error
    mov [server_fd], eax
    
    call print_str
    dd msg_listening
    
    ; Listen on server
    mov eax, SYSCALL_LISTEN
    mov ebx, [server_fd]
    mov ecx, 5              ; backlog
    int 0x80
    cmp eax, 0
    jl .error
    
    ; ===== CLIENT CONNECT =====
    ; Create client socket
    mov eax, SYSCALL_SOCKET
    mov ebx, SOCK_TYPE_STREAM
    int 0x80
    cmp eax, 0
    jl .error
    mov [client_fd], eax
    
    ; Connect to server
    mov eax, SYSCALL_CONNECT
    mov ebx, [client_fd]
    mov ecx, server_path
    int 0x80
    cmp eax, 0
    jl .error
    
    ; ===== SERVER ACCEPT =====
    call print_str
    dd msg_waiting
    
    mov eax, SYSCALL_ACCEPT
    mov ebx, [server_fd]
    int 0x80
    cmp eax, 0
    jl .error
    mov [client_fd], eax    ; Reuse client_fd for accepted connection
    
    call print_str
    dd msg_accepted
    
    ; ===== CLIENT SEND =====
    ; Send message from client
    mov eax, SYSCALL_SEND
    mov ebx, [client_fd]
    mov ecx, client_msg
    mov edx, 18             ; length
    xor esi, esi            ; flags
    int 0x80
    
    ; ===== SERVER RECEIVE =====
    ; Receive on accepted connection
    mov eax, SYSCALL_RECV
    mov ebx, [client_fd]
    mov ecx, recv_buffer
    mov edx, 256
    xor esi, esi
    int 0x80
    
    ; Print received message
    call print_str
    dd msg_received
    
    mov eax, SYSCALL_PRINT
    mov ebx, recv_buffer
    int 0x80
    
    call print_str
    dd newline
    
    ; ===== SERVER SEND RESPONSE =====
    call print_str
    dd msg_sending
    
    mov eax, SYSCALL_SEND
    mov ebx, [client_fd]
    mov ecx, server_response
    mov edx, 18
    xor esi, esi
    int 0x80
    
    ; ===== CLEANUP =====
    ; Close client connection
    mov eax, SYSCALL_CLOSE
    mov ebx, [client_fd]
    int 0x80
    
    ; Close server socket
    mov eax, SYSCALL_CLOSE
    mov ebx, [server_fd]
    int 0x80
    
    ; Unlink server socket
    mov eax, SYSCALL_UNLINK
    mov ebx, server_path
    int 0x80
    
    ; Success!
    call print_str
    dd msg_done
    
    mov eax, SYSCALL_EXIT
    xor ebx, ebx
    int 0x80

.error:
    call print_str
    dd msg_error
    mov eax, SYSCALL_EXIT
    mov ebx, 1
    int 0x80

; Helper to print string
print_str:
    pop esi                 ; Return address
    pop ebx                 ; String pointer
    push ebx
    push esi
    
    mov eax, SYSCALL_PRINT
    int 0x80
    ret

section .rodata
    msg_done db "Socket demo completed successfully!", 10, 0
    msg_error db "Socket demo failed!", 10, 0
