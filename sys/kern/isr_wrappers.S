.section .text
.code32

/* ============================================================
 * ISR macros
 * ============================================================ */

.macro ISR_ERROR_CODE num
    .globl isr\num
isr\num:
    pushl $\num           /* push interrupt number */
    jmp handle_isr_common
.endm

.macro ISR_NO_ERROR_CODE num
    .globl isr\num
isr\num:
    pushl $0              /* dummy error code */
    pushl $\num           /* interrupt number */
    jmp handle_isr_common
.endm

/* ============================================================
 * ISR table
 * ============================================================ */

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
ISR_NO_ERROR_CODE  128

/* ============================================================
 * Externals
 * ============================================================ */

.extern handle_isr
.extern terminal_print_register_value

/* ============================================================
 * Register save / restore macros
 * ============================================================ */

.macro SAVE_REGS
    pushal

    movw %ds, %ax
    pushl %eax

    movw %es, %ax
    pushl %eax
    movw %fs, %ax
    pushl %eax
    movw %gs, %ax
    pushl %eax

    pushl %ebx
    movw $0x10, %bx        /* kernel data segment */
    movw %bx, %ds
    movw %bx, %es
    movw %bx, %fs
    movw %bx, %gs
    popl %ebx
.endm

.macro RESTORE_REGS
    popl %gs
    popl %fs
    popl %es
    popl %ds
    popal
.endm

/* ============================================================
 * Common ISR handler
 * ============================================================ */

.globl handle_isr_common
handle_isr_common:
    SAVE_REGS
    call handle_isr
    RESTORE_REGS
    addl $8, %esp          /* drop error code + int number */
    iret
