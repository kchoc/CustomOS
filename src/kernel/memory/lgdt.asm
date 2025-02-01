section .text
global load_gdt

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
