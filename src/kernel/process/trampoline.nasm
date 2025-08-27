; trampoline.s  (nasm)
; assemble with: nasm -f bin trampoline.s -o trampoline.bin
[bits 16]
[org 0x0]                 ; we will copy this binary to physical trampoline address

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7c00        ; temporary stack (must be in low memory)
    ; load a tiny GDT
    lgdt [gdt_descriptor]

    ; turn on protected mode: set CR0.PE
    mov eax, cr0
    or eax, 1
    mov cr0, eax

    ; far jump to clear prefetched instruction
    jmp 0x08:protected_entry

[BITS 32]
protected_entry:
    ; set up segment registers to flat descriptors
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; set a usable stack in high low memory - must be safe
    mov esp, 0x9FC00      ; safe stack in low memory; you can change

    ; Call into C entry point (extern symbol)
    extern ap_entry
    call ap_entry

    ; If ap_entry returns, loop forever
.spin:
    hlt
    jmp .spin

; GDT: null, code (0x08), data (0x10)
gdt_start:
gdt_null:    dd 0, 0
gdt_code:    dw 0xFFFF, dw 0x0000, db 0x00, db 0x9A, db 0xCF, db 0x00
gdt_data:    dw 0xFFFF, dw 0x0000, db 0x00, db 0x92, db 0xCF, db 0x00
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start
