.code16
.global _start

.equ BOOT1_OFFSET,    0x7C00
.equ PAGE_DIR,        0x0007D000
.equ BOOT_PAGE_TABLE, 0x0007E000
.equ KERN_PAGE_TABLE, 0x0007F000
.equ KERN_LOAD_SEG,   0x4000
.equ KERNEL_ADDRESS,  0x00040000

_start:
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

load_kernel:
    movw $kernel_name, %bx
    movw $KERN_LOAD_SEG, %cx   # Load kernel to 0x10000 (just after page tables)
    call (LOAD_FILE+BOOT1_OFFSET)

kernel_loaded:

# --------------------------------------------------
# Setup Paging & Protected Mode
# --------------------------------------------------
    cli
    lgdt gdt_desc

    movl %cr0, %eax
    orl $0x00000001, %eax    # enable PE
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
    movl $(KERNEL_ADDRESS | 3), %eax
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
    
kernel_name:
    .ascii "KERNEL  BIN" # 11 bytes, space-padded

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
