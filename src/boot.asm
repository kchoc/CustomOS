bits 32

section .multiboot
    dd 0x1BADB002            ; Multiboot header magic
    dd 0x0                   ; Flags
    dd - (0x1BADB002 + 0x0)  ; Checksum

section .text
global start
extern main

start:
    cli                      ; Disable interrupts
    mov esp, stack_space     ; Set stack pointer

    ; Set up GDT
    lgdt [gdt_descriptor]

    ; Enter protected mode
    mov eax, cr0
    or eax, 1
    mov cr0, eax

    ; Far jump to flush the prefetch queue and load CS
    jmp 0x08:protected_mode

bits 32

protected_mode:
    ; Set up segment registers
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Call the kernel entry point
    call main

.halt:
    hlt                      ; Halt the CPU
    jmp .halt                ; Infinite loop

section .bss
align 16
resb 16384                  ; 16KB stack
stack_space:

section .data
gdt_start:
    ; Null descriptor
    dd 0x0
    dd 0x0

    ; Code segment descriptor
    dw 0xffff
    dw 0x0
    db 0x0
    db 10011010b
    db 11001111b
    db 0x0

    ; Data segment descriptor
    dw 0xffff
    dw 0x0
    db 0x0
    db 10010010b
    db 11001111b
    db 0x0

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start