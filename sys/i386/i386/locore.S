#include "machine/asmmacros.h"

.set STACK_SIZE, 16384
.set BOOTINFO_SIZE, 4

/* -----------------------------
 * Stack
 * ----------------------------- */
.section .bootstrap_stack, "aw", @nobits
.data

stack_bottom:
    .skip STACK_SIZE
stack_top:

.globl bootinfo
bootinfo:
    .space BOOTINFO_SIZE

.text
ENTRY(btext)
    /* Save the bootinfo pointer (passed in %ebx) for later use */
    movl %ebx, bootinfo

    /* Disable interrupts */
    cli

    /* Set up stack */
    movl $stack_top, %esp
    movl %esp, %ebp

    /* Optional: clear BSS after paging is set up
    call clear_bss */

    /* Jump into higher-level kernel init */
    call init386

END(btext)

halt:
    cli
    hlt
    jmp halt

/* Optional BSS clearing routine (call after paging active) */
clear_bss:
    movl $__bss_start, %edi
    movl $__bss_end, %ecx
    subl %edi, %ecx
    xorl %eax, %eax
    rep stosb
    ret
