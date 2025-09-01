; ---------------------------------------------------------------------------
; 16-bit real mode entry
; ---------------------------------------------------------------------------
section .trampoline16 progbits align=16
    bits 16
    global trampoline_start

trampoline_start:
    cli
    cld

    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax

    ; Check to see if this location is reached
    mov word [0x7008], 0x1234

    mov sp, 0x7C00           ; temporary RM stack

    lgdt [gdt_descriptor]

    ; enable protected mode
    mov eax, cr0
    or  eax, 1
    mov cr0, eax

    jmp 0x08:protected_entry ; far jump => 32-bit

; ---------------------------------------------------------------------------
; 32-bit protected mode entry
; ---------------------------------------------------------------------------
section .trampoline32 progbits align=16
    bits 32

protected_entry:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    mov esp, 0x80000        ; Load stack address from known location

    ; Enable paging by loading the page directory base register (CR3)
    mov eax, dword [0x7004] ; Page directory base address at physical 0x7004
    mov cr3, eax

    ; Enable paging by setting the PG bit in CR0
    mov eax, cr0
    or  eax, 0x80000000
    mov cr0, eax

    mov eax, [0x7000]        ; AP entry pointer at physical 0x7000
    call eax

.spin:
    hlt
    jmp .spin

; ---------------------------------------------------------------------------
; GDT (shared between 16/32-bit sections)
; ---------------------------------------------------------------------------
section .trampoline_gdt progbits align=8
    bits 16

gdt_start:
    dq 0x0000000000000000        ; null descriptor
    ; code: limit=0xFFFFF, base=0x0, access=0x9A, gran=0xCF
    dq 0x00CF9A000000FFFF        ; bytes: ff ff 00 00 00 9a cf 00
    ; data: limit=0xFFFFF, base=0x0, access=0x92, gran=0xCF
    dq 0x00CF92000000FFFF        ; bytes: ff ff 00 00 00 92 cf 00
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1   ; limit
    dd gdt_start                 ; base