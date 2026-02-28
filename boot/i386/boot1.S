.code16
.global _start

# --------------------------------------------------
# Configuration
# --------------------------------------------------
.equ KERNEL_LBA,      2049
.equ KERNEL_LOAD,     0x00010000    # initial load at 64KB
.equ KERNEL_LOAD_SEG, 0x1000        # segment for initial load (64KB)
.equ KERNEL_DEST,     0x00100000    # copy kernel to above 1MB
.equ PAGE_DIR,        0x0007D000
.equ BOOT_PAGE_TABLE, 0x0007E000
.equ KERN_PAGE_TABLE, 0x0007F000
.equ KERNEL_SIZE,     KERNEL_SECTORS*512   # kernel size in bytes
.equ READ_QUANTUM,    64
.equ SEGMENT_INCREMENT, 0x1000 / 128 * READ_QUANTUM  # how much to increment segment for each read

# --------------------------------------------------
# Entry
# --------------------------------------------------
_start:
    cli
    xorw %ax, %ax
    movw %ax, %ds
    movw %ax, %ss
    movw %cs, %ax
    movw %ax, %ds
    movw $0x7C00, %sp
    push %ds

    movb %dl, boot_drive

# --------------------------------------------------
# Check INT 13h Extensions (EDD)
# --------------------------------------------------
    movb $0x41, %ah
    movw $0x55AA, %bx
    movb boot_drive, %dl
    int $0x13
    jc no_lba
    cmpw $0xAA55, %bx
    jne no_lba
    testw $1, %cx
    jz no_lba

# --------------------------------------------------
# Enable A20
# --------------------------------------------------
    inb $0x92, %al
    orb $2, %al
    outb %al, $0x92

# --------------------------------------------------
# Check INT 15h for memory size (0xE820)
# --------------------------------------------------
    push %es # I should probably find a better way to do this, but I want to be able to use es:di for the memory map buffer
    xorw %ax, %ax
    movw %ax, %es
    movw $(memory_map), %di
    xorl %ebx, %ebx          # continuation value
    xorl %esi, %esi          # entry counter

next_entry:
    movl $0xE820, %eax
    movl $0x534D4150, %edx   # "SMAP"
    movl $24, %ecx
    int $0x15
    jc done_memory

    addw $24, %di
    inc %si
    cmp $0, %ebx
    jne next_entry

done_memory:
    pop %es
    movl %esi, (bootinfo + 4)   # store number of entries in bootinfo

# --------------------------------------------------
# Load Kernel Directly Above 64KB in 64KB Chunks
# --------------------------------------------------
    movw $KERNEL_LOAD_SEG, %di   # destination segment
    movw $KERNEL_SECTORS, %cx    # remaining sectors
    movw $KERNEL_LBA, %bx        # starting LBA

read_loop:
    test %cx, %cx
    jz kernel_loaded

    movw $READ_QUANTUM, %ax
    cmp %cx, %ax        # ax - cx
    jbe 1f              # if ax <= cx → keep ax (quantum)
    mov %cx, %ax        # else ax = remaining
1:
    mov %ax, %dx

    # setup DAP
    lea dap, %si
    movb $0x10, dap
    movb $0, dap+1
    movw %dx, dap+2
    movw $0, dap+4
    movw %di, dap+6
    movl %ebx, dap+8
    movl $0, dap+12

    movb $0x42, %ah
    movb boot_drive, %dl
    int $0x13
    jc disk_error

    addw dap+2, %bx   # increment LBA
    subw dap+2, %cx   # decrement remaining sectors
    addw $SEGMENT_INCREMENT, %di

    jmp read_loop

kernel_loaded:

# --------------------------------------------------
# Copy kernel from 64KB to 1MB
# --------------------------------------------------
    movl $KERNEL_LOAD, %esi        # source: 64KB
    movl $KERNEL_DEST, %edi        # destination: 1MB
    movl $KERNEL_SIZE, %ecx
    rep movsb

# --------------------------------------------------
# Setup Paging & Protected Mode
# --------------------------------------------------
    cli
    lgdt gdt_desc

    movl %cr0, %eax
    orl $0x00000001, %eax    # enable PG + PE
    movl %eax, %cr0

    ljmp $0x08, $protected_entry

.code32
protected_entry:
    movl $0x10, %eax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    movw %ax, %ss

    # Clear Page Directory
    movl $PAGE_DIR, %edi
    xorl %eax, %eax
    movl $1024, %ecx
    rep stosl

    # Build identity-mapped page table for bootloader (1MB)
    movl $BOOT_PAGE_TABLE, %edi
    movl $0x00000003, %eax
    movl $16, %ecx

boot_pt_fill:
    stosl
    addl $0x1000, %eax
    loop boot_pt_fill

    # Build higher-half page table for kernel (4MB)
    movl $KERN_PAGE_TABLE, %edi
    movl $0x00010003, %eax
    movl $1023, %ecx

kern_pt_fill:
    stosl
    addl $0x1000, %eax
    loop kern_pt_fill

    # Map VGA framebuffer (0xB8000) to last kB of higher-half
    movl $(0xB8000 | 3), (KERN_PAGE_TABLE + (1023*4))

    # PDE[0] = identity, PDE[768] = higher-half
    movl $(BOOT_PAGE_TABLE | 3), (PAGE_DIR)
    movl $(KERN_PAGE_TABLE | 3), (PAGE_DIR + 768*4)
    movl $(PAGE_DIR        | 3), (PAGE_DIR + 1023*4)   # recursive mapping

    movl $PAGE_DIR, %eax
    movl %eax, %cr3

    movl %cr0, %eax
    orl $0x80000000, %eax    # enable PG + PE
    movl %eax, %cr0

    movl $bootinfo, %ebx   # pass bootinfo pointer in ebx

    movl $0xC0000000, %eax   # higher-half entry point
    jmp *%eax

# --------------------------------------------------
# Errors
# --------------------------------------------------
.code16
disk_error:
    xorw %ax, %ax
no_lba:
    cli
hang:
    hlt
    jmp hang

# --------------------------------------------------
# Data
# --------------------------------------------------
boot_drive:
    .byte 0

.align 4
dap:
    .byte 0x10
    .byte 0
    .word 0
    .word 0
    .word 0
    .quad 0

.align 4
bootinfo:
    .long memory_map
    .long 0 # Number of memory map entries, will be filled in by the bootloader

.align 8
gdt:
    .quad 0x0000000000000000
    .quad 0x00CF9A000000FFFF
    .quad 0x00CF92000000FFFF
gdt_end:

gdt_desc:
    .word gdt_end - gdt - 1
    .long gdt

memory_map: # No need to reserve space here since we just write directly to it
