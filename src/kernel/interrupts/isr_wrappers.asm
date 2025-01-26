bits 32

global isr_keyboard_wrapper
extern isr_keyboard_handler

isr_keyboard_wrapper:
    cli                         ; Disable interrupts    
    push 0                      ; Push error code (if none exists)
    push 0                      ; Push interrupt number
    call isr_keyboard_handler   ; Call the ISR
    add esp, 8                  ; Clean up stack
    sti                         ; Re-enable interrupts
    iret                        ; Return from interrupt

global isr_divide_by_zero_wrapper
global isr_debug_wrapper
global isr_non_maskable_interrupt_wrapper
global isr_breakpoint_wrapper
global isr_overflow_wrapper
global isr_bound_range_exceeded_wrapper
global isr_invalid_opcode_wrapper
global isr_device_not_available_wrapper
global isr_double_fault_wrapper
global isr_invalid_tss_wrapper
global isr_segment_not_present_wrapper
global isr_stack_segment_fault_wrapper
global isr_general_protection_fault_wrapper
global isr_page_fault_wrapper
global isr_fpu_error_wrapper
global isr_alignment_check_wrapper
global isr_machine_check_wrapper
global isr_simd_floating_point_wrapper

extern isr_divide_by_zero
extern isr_debug
extern isr_non_maskable_interrupt
extern isr_breakpoint
extern isr_overflow
extern isr_bound_range_exceeded
extern isr_invalid_opcode
extern isr_device_not_available
extern isr_double_fault
extern isr_invalid_tss
extern isr_segment_not_present
extern isr_stack_segment_fault
extern isr_general_protection_fault
extern isr_page_fault
extern isr_fpu_error
extern isr_alignment_check
extern isr_machine_check
extern isr_simd_floating_point

isr_divide_by_zero_wrapper:
    cli
    push 0
    push 0
    call isr_divide_by_zero
    add esp, 8
    sti
    iret

isr_debug_wrapper:
    cli
    push 0
    push 1
    call isr_debug
    add esp, 8
    sti
    iret

isr_non_maskable_interrupt_wrapper:
    cli
    push 0
    push 2
    call isr_non_maskable_interrupt
    add esp, 8
    sti
    iret

isr_breakpoint_wrapper:
    cli
    push 0
    push 3
    call isr_breakpoint
    add esp, 8
    sti
    iret

isr_overflow_wrapper:
    cli
    push 0
    push 4
    call isr_overflow
    add esp, 8
    sti
    iret

isr_bound_range_exceeded_wrapper:
    cli
    push 0
    push 5
    call isr_bound_range_exceeded
    add esp, 8
    sti
    iret

isr_invalid_opcode_wrapper:
    cli
    push 0
    push 6
    call isr_invalid_opcode
    add esp, 8
    sti
    iret

isr_device_not_available_wrapper:
    cli
    push 0
    push 7
    call isr_device_not_available
    add esp, 8
    sti
    iret

isr_double_fault_wrapper:
    cli
    push 0
    push 8
    call isr_double_fault
    add esp, 8
    sti
    iret

isr_invalid_tss_wrapper:
    cli
    push 0
    push 10
    call isr_invalid_tss
    add esp, 8
    sti
    iret

isr_segment_not_present_wrapper:
    cli
    push 0
    push 11
    call isr_segment_not_present
    add esp, 8
    sti
    iret

isr_stack_segment_fault_wrapper:
    cli
    push 0
    push 12
    call isr_stack_segment_fault
    add esp, 8
    sti
    iret

isr_general_protection_fault_wrapper:
    cli
    push 0
    push 13
    call isr_general_protection_fault
    add esp, 8
    sti
    iret

isr_page_fault_wrapper:
    cli
    push 0
    push 14
    call isr_page_fault
    add esp, 8
    sti
    iret

isr_fpu_error_wrapper:
    cli
    push 0
    push 16
    call isr_fpu_error
    add esp, 8
    sti
    iret

isr_alignment_check_wrapper:
    cli
    push 0
    push 17
    call isr_alignment_check
    add esp, 8
    sti
    iret

isr_machine_check_wrapper:
    cli
    push 0
    push 18
    call isr_machine_check
    add esp, 8
    sti
    iret

isr_simd_floating_point_wrapper:
    cli
    push 0
    push 19
    call isr_simd_floating_point
    add esp, 8
    sti
    iret