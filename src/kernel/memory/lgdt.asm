section .text
global load_gdt
global load_tss

load_gdt:
    mov eax, [esp + 4]   ; Load GDT descriptor argument from stack
    lgdt [eax]           ; Load the GDT

    ; Far jump to reload CS
    jmp 0x08:.flush

.flush:
    mov ax, 0x10         ; Load data segment selectors
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ret                  ; Return to caller

load_tss:
    mov ax, [esp + 4]   ; Load TSS segment selector argument from stack
    ltr ax              ; Load the TSS segment selector into TR register
    ret
