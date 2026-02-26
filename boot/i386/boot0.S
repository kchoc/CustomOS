.code16
.global _start

.set LOAD_ADDR,     0x7C00
.set RELOC_ADDR,    0x0600
.set STACK_TOP,     0x7C00
.set PART_ENTRY_SZ, 16
.set FREEBSD_ID,    0xA5

_start:
    cli
    xorw %ax,%ax
    movw %ax,%ds
    movw %ax,%es
    movw %ax,%ss
    movw $STACK_TOP,%sp
    sti

    movb %dl, boot_drive

# -------------------------------
# Relocate to 0x0600
# -------------------------------
    movw $LOAD_ADDR, %si
    movw $RELOC_ADDR, %di
    movw $256, %cx
    rep movsw
    ljmp $0, $(relocated-LOAD_ADDR+RELOC_ADDR)

# ===============================
# Running at 0x0600
# ===============================
relocated:
    movb boot_drive, %dl

    movw $(RELOC_ADDR + 0x1BE), %si  # Point to partition table
    movw $4, %cx

# -------------------------------
# Find active partition
# -------------------------------
find_active:
    cmpb $0x80, (%si)
    je found
    addw $PART_ENTRY_SZ, %si
    loop find_active

    jmp partition_not_found

found:
    movw %si, %bx

# -------------------------------
# Check LBA support
# -------------------------------
    movb $0x41, %ah
    movw $0x55AA, %bx
    int $0x13
    jc use_chs
    cmpw $0xAA55, %bx
    jne use_chs

# -------------------------------
# LBA read (INT 13h AH=42h)
# -------------------------------
    movw 8(%si), %ax         # Starting LBA lower 16 bits
    movw 10(%si), %dx        # Starting LBA upper 16 bits
    movw %ax, dap_lba
    movw %dx, dap_lba+2

    # Unneeded, already set by compiler initialization
    # movb $16, dap_size
    # movb $0, dap_reserved

    movw $1, dap_sectors
    movw $0x7C00, dap_offset
    movw $0x0000, dap_segment
    movl $0, dap_lba_high

    movb $0x42, %ah
    movw $dap, %si
    movb boot_drive, %dl
    int $0x13
    jc disk_error
    jmp boot_partition

# -------------------------------
# CHS fallback
# -------------------------------
use_chs:
    movb 1(%bx), %dh
    movb 2(%bx), %cl
    movb 3(%bx), %ch
    movb boot_drive, %dl

    xorw %ax,%ax
    movw %ax,%es
    movw $0x7C00,%bx

    movb $0x02,%ah
    movb $1,%al
    int $0x13
    jc disk_error

# -------------------------------
# Jump to partition boot sector
# -------------------------------
boot_partition:
    ljmp $0,$0x7C00

# -------------------------------
# Error handler
# -------------------------------
disk_error:
    cli
    # Preserve AH (error code) for debugging
    mov %ax, %bx
    # Output to serial port for debugging
    mov $'E', %al
    call .com1_output
    # Push the int 13h error code from AH
    mov %bx, %ax
    call .com1_output
    jmp .hang

partition_not_found:
    cli
    # Output to serial port for debugging
    mov $'N', %al
    call .com1_output
    jmp .hang

.hang:
    hlt
    jmp .hang

.com1_output:
    mov $0x01, %ah    # Function 01h: transmit character
    mov $0x00, %dx    # COM1 port
    int $0x14
    ret

# ===============================
# Data (inside 512 bytes)
# ===============================
boot_drive:
    .byte 0

.align 16
dap:
dap_size:      .byte 16
dap_reserved:  .byte 0
dap_sectors:   .word 0
dap_offset:    .word 0
dap_segment:   .word 0
dap_lba:       .long 0
dap_lba_high:  .long 0

# ===============================
# Boot Signature and Partition Table
# ===============================
.org 0x1BE

# Partition 1 (Active)
.byte 0x80              # Bootable
.byte 0x00,0x02,0x00    # Starting CHS (fake but valid)
.byte 0xFF              # Partition type (0xFF = unknown)
.byte 0xFF,0xFF,0xFF    # Ending CHS (maxed)

.long 2048              # Starting LBA
.long 100000            # Size in sectors

# Partition 2 (empty)
.byte 0x00
.byte 0x00,0x00,0x00
.byte 0x00
.byte 0x00,0x00,0x00
.long 0
.long 0

# Partition 3 (empty)
.byte 0x00
.byte 0x00,0x00,0x00
.byte 0x00
.byte 0x00,0x00,0x00
.long 0
.long 0

# Partition 4 (empty)
.byte 0x00
.byte 0x00,0x00,0x00
.byte 0x00
.byte 0x00,0x00,0x00
.long 0
.long 0

# Boot signature
.org 0x1FE
.word 0xAA55
