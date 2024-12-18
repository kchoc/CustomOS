bits 32

global keyboard_isr_wrapper
extern keyboard_isr_handler

keyboard_isr_wrapper:
    cli                         ; Disable interrupts    
    push 0                      ; Push error code (if none exists)
    push 0                      ; Push interrupt number
    call keyboard_isr_handler   ; Call the ISR
    add esp, 8                  ; Clean up stack
    sti                         ; Re-enable interrupts
    iret                        ; Return from interrupt
