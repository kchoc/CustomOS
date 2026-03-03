.code16gcc
.global _start

# --------------------------------------------------
# Configuration
# --------------------------------------------------
.equ FAT_TABLE_SEG,   0x1300
.equ RDT_LOAD_SEG,    0x1100
.equ BOOT2_DEST_SEG,  0x7e0
.equ BOOT2_DEST,      BOOT2_DEST_SEG << 4

# --------------------------------------------------
# MBR Setup (BIOS Parameter Block)
# --------------------------------------------------
_start:
    jmp boot_start
    nop

    os_name:
    .ascii "OS_FAT16" # OEM Name
    bytes_per_sector:
    .word 512
    sectors_per_cluster:
    .byte 1
    reserved_sectors:
    .word 1
    num_fats:
    .byte 2
    max_root_entries:
    .word 512
    total_sectors_short:
    .word 63488
    media_descriptor:
    .byte 0xF8
    sectors_per_fat:
    .word 246
    sectors_per_track:
    .word 63
    num_heads:
    .word 16
    hidden_sectors:
    .long 2048
    total_sectors_long:
    .long 0
    drive_number:
    .byte 0x00
    reserved:
    .byte 0
    boot_signature: 
    .byte 0x29
    volume_id:
    .long 0x12345678
    volume_label:
    .ascii "NO NAME    "
    fs_type:
    .ascii "FAT16   "

# --------------------------------------------------
# Entry
# --------------------------------------------------
boot_start:
    cli
    xorw %ax, %ax
    movw %ax, %ds
    movw %ax, %ss
    movw %cs, %ax
    movw %ax, %ds
    movw $0x7C00, %sp
    pushw %ds

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
# Read the FAT root directory
# read_sectors:
#   START (BX) = num_fats * sectors_per_fat + reserved_sectors + hidden_sectors
#   COUNT (CX) = max_root_entries * 32 / 512 (number of sectors to read)
#   DEST  (DX) = RDT_LOAD_SEG
# --------------------------------------------------
    movw max_root_entries, %ax
    shlw $5, %ax # 32 bytes per entry
    movw $512, %bx
    xorw %dx, %dx
    divw %bx
    movw %ax, %cx # number of sectors to read
    
    xorw %ax, %ax
    movb num_fats, %al
    movw sectors_per_fat, %bx
    mulw %bx
    addw reserved_sectors, %ax # add reserved sectors to get to FAT tables
    addl hidden_sectors, %eax # add hidden sectors for LBA calculation
    movw %ax, %bx               # starting LBA of root directory

    movw $RDT_LOAD_SEG, %dx

    call read_sector_lba

# --------------------------------------------------
# Read the FAT tables (just after root directory)
# read_sectors:
#   START (BX) = reserved_sectors + hidden_sectors
#   COUNT (CX) = num_fats * sectors_per_fat
#   DEST  (DX) = FAT_TABLE_SEG
# --------------------------------------------------
    xorw %ax, %ax
    movb num_fats, %al
    movw sectors_per_fat, %bx
    mulw %bx
    # movw %ax, %cx # number of sectors to read
    movw $64, %cx # read 64 sectors to load both FAT tables (246 sectors each is too much for the bootloader)
    
    movw reserved_sectors, %ax
    addl hidden_sectors, %eax # add hidden sectors for LBA calculation
    movw %ax, %bx # starting LBA of FAT tables

    movw $FAT_TABLE_SEG, %dx

    call read_sector_lba

# --------------------------------------------------
# Find BOOT2.BIN in the root directory
# --------------------------------------------------

load_boot2:
    movw $boot2_name, %bx
    movw $BOOT2_DEST_SEG, %cx
    call load_file
    # For some reason the load_file return starts executing garbage so buffer the jump to BOOT2.BIN
    xorw %ax, %ax
    xorw %bx, %bx
    xorw %cx, %cx
    xorw %dx, %dx
    jmp BOOT2_DEST

# --------------------------------------------------
# Load file by name from root directory
# Inputs:
#   BX = pointer to filename (11 bytes, space-padded)
#   CX = destination segment
# --------------------------------------------------
load_file:
    pushw %ds
    pushw %es
    pushw %cx

    # Use the ES to point to the root directory for easy access to entries
    movw $RDT_LOAD_SEG, %ax
    # Not sure why i cant use ax -> es
    pushw %ax
    popw %es

    xorw %ax, %ax
    movw %ax, %ds          # DS = 0 for filename comparison
    xorw %si, %si          # start at first root directory entry

search_rdt:
    # Check entry presence
    movw %es:(%si), %ax        # read first 2 bytes of filename
    cmpw $0, %ax
    je file_not_found

    # Check entry type
    movb %es:11(%si), %al      # attributes
    cmpb $0x20, %al        # 0x20 = archive (normal file)
    jne next_entry

    # Check entry name
    pushw %si
    pushw %bx
    movw $11, %cx
compare_name:
    movb %es:(%si), %al
    cmpb (%bx), %al
    jne not_match
    incw %si
    incw %bx
    loop compare_name
    jmp file_found
not_match:
    popw %bx
    popw %si

next_entry:
    addw $32, %si          # next directory entry
    jmp search_rdt

file_found:
    popw %bx
    popw %si
    movw %es:26(%si), %di      # starting cluster
    movw $FAT_TABLE_SEG, %ax
    movw %ax, %es
    popw %si

follow_clusters:
    movw %di, %ax
    subw $2, %ax                        # cluster numbers start at 2
    xorw %bx, %bx
    movb sectors_per_cluster, %bl
    mulw %bx                           # AX = (cluster_number - 2) * sectors_per_cluster

    # Add reserved sectors + hidden sectors
    addw reserved_sectors, %ax
    addl hidden_sectors, %eax # add hidden sectors for LBA calculation
    movw %ax, %bx

    # Add FAT table sectors
    xorw %ax, %ax
    movb num_fats, %al
    movw sectors_per_fat, %cx
    mulw %cx                            # DX:AX = sectors_per_fat * num_fats
    addw %ax, %bx

    # Add root directory sectors
    xorw %dx, %dx
    movw max_root_entries, %ax
    shlw $5, %ax # 32 bytes per entry
    movw $512, %cx
    divw %cx
    addw %ax, %bx

    # Number of sectors to read
    xorw %cx, %cx
    movb sectors_per_cluster, %cl

    movw %si, %dx
    pushw %si

    call read_sector_lba

    popw %si

    # Update file destination offset
    addw $32, %si # +32 segments = 512 bytes = 1 sector

    # -----------------------------
    # Read next cluster from FAT table
    # -----------------------------
    movw %di, %ax                         # AX = current cluster number
    shlw $1, %ax                          # FAT16 entry = 2 bytes
    movw %ax, %bx
    movw %es:(%bx), %di                   # read FAT entry for current cluster
    cmpw $0xFFFF, %di
    je file_loaded
    cmpw $0, %di
    je file_not_found
    jmp follow_clusters

file_loaded:
    popw %es
    popw %ds
    ret

file_not_found:
    jmp hang

# --------------------------------------------------
# Errors
# --------------------------------------------------
disk_error:
no_lba:
    cli
hang:
    hlt
    jmp hang

# --------------------------------------------------
# Read sector using LBA (OR CHS fallback if LBA not supported TODO)
# --------------------------------------------------
# Inputs: BX = LBA address, CX = number of sectors, DX = destination segment
read_sector_lba:
    lea dap, %si
    movb $0x10, dap
    movb $0, dap+1
    movw %cx, dap+2
    movw $0, dap+4
    movw %dx, dap+6
    movl %ebx, dap+8
    movl $0, dap+12

    movb $0x42, %ah
    movb boot_drive, %dl
    int $0x13
    jc disk_error
    ret

# --------------------------------------------------
# Data
# --------------------------------------------------
boot_drive:
    .byte 0

load_addr:
    .long BOOT2_DEST_SEG

boot2_name:
    .ascii "BOOT2   BIN" # 11 bytes, space-padded

.align 4
dap:
    .byte 0x10  # size of DAP structure (16 bytes)
    .byte 0     # reserved
    .word 0     # number of sectors to read
    .word 0     # transfer buffer offset
    .word 0     # transfer buffer segment
    .quad 0     # starting LBA address

.space 510 - (. - _start), 0
    .word 0xAA55