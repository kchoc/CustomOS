; NASM macro
%macro ISR_ERROR_CODE 1
  global isr%1
  isr%1:
    push dword %1         ; push the interrupt number
    jmp handle_isr_common
%endmacro

; NASM macro
%macro ISR_NO_ERROR_CODE 1
  global isr%1
  isr%1:
    push dword 0          ; push a dummy error code just because to keep struct Registers the same as the above macro
    push dword %1         ; push the interrupt number
    jmp handle_isr_common
%endmacro

        
ISR_NO_ERROR_CODE  0
ISR_NO_ERROR_CODE  1
ISR_NO_ERROR_CODE  2
ISR_NO_ERROR_CODE  3
ISR_NO_ERROR_CODE  4
ISR_NO_ERROR_CODE  5
ISR_NO_ERROR_CODE  6
ISR_NO_ERROR_CODE  7
ISR_ERROR_CODE     8
ISR_NO_ERROR_CODE  9
ISR_ERROR_CODE     10
ISR_ERROR_CODE     11
ISR_ERROR_CODE     12
ISR_ERROR_CODE     13
ISR_ERROR_CODE     14
ISR_NO_ERROR_CODE  15
ISR_NO_ERROR_CODE  16
ISR_NO_ERROR_CODE  17
ISR_NO_ERROR_CODE  18
ISR_NO_ERROR_CODE  19
ISR_NO_ERROR_CODE  20
ISR_NO_ERROR_CODE  21
ISR_NO_ERROR_CODE  22
ISR_NO_ERROR_CODE  23
ISR_NO_ERROR_CODE  24
ISR_NO_ERROR_CODE  25
ISR_NO_ERROR_CODE  26
ISR_NO_ERROR_CODE  27
ISR_NO_ERROR_CODE  28
ISR_NO_ERROR_CODE  29
ISR_NO_ERROR_CODE  30
ISR_NO_ERROR_CODE  31
ISR_NO_ERROR_CODE  32
ISR_NO_ERROR_CODE  33
ISR_NO_ERROR_CODE  64
ISR_NO_ERROR_CODE 128

extern terminal_print_register_value

%macro  SAVE_REGS 0
        pushad
        mov eax, ds
        push eax               ; 4 bytes
        
        ; Prtnt segment registers for debugging
        ; push eax
        ; call terminal_print_register_value
        ; add esp, 4

        mov eax, es
        push eax
        mov eax, fs
        push eax
        mov eax, gs
        push eax

        push ebx
        mov bx, 0x10 ; load the kernel data segment descriptor
        mov ds, bx
        mov es, bx
        mov fs, bx
        mov gs, bx
        pop ebx
%endmacro

%macro  RESTORE_REGS 0
        pop gs
        pop fs
        pop es
        pop ds
        popad
%endmacro


extern handle_isr

handle_isr_common:
    SAVE_REGS
    call handle_isr
    RESTORE_REGS
    add esp, 8     ; deallocate the error code and the interrupt number
    iret           ; pops CS, EIP, EFLAGS and also SS, and ESP if privilege change occurs
    