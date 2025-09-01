#ifndef ELF_H
#define ELF_H

#include <stdint.h>

#define EI_NIDENT 16

// ============================
// ELF Header
// ============================
typedef struct {
    unsigned char e_ident[EI_NIDENT]; // ELF identification
    uint16_t e_type;      // Object file type
    uint16_t e_machine;   // Architecture (3 = x86)
    uint32_t e_version;   // Object file version
    uint32_t e_entry;     // Entry point virtual address
    uint32_t e_phoff;     // Program header table file offset
    uint32_t e_shoff;     // Section header table file offset
    uint32_t e_flags;     // Processor-specific flags
    uint16_t e_ehsize;    // ELF header size
    uint16_t e_phentsize; // Program header table entry size
    uint16_t e_phnum;     // Program header table entry count
    uint16_t e_shentsize; // Section header table entry size
    uint16_t e_shnum;     // Section header table entry count
    uint16_t e_shstrndx;  // Section header string table index
} Elf32_Ehdr;

// ============================
// Program Header
// ============================
typedef struct {
    uint32_t p_type;   // Segment type
    uint32_t p_offset; // Segment file offset
    uint32_t p_vaddr;  // Segment virtual address
    uint32_t p_paddr;  // Physical address (ignore for now)
    uint32_t p_filesz; // Size of segment in file
    uint32_t p_memsz;  // Size of segment in memory
    uint32_t p_flags;  // Segment flags
    uint32_t p_align;  // Alignment
} Elf32_Phdr;

// ============================
// Section Header (optional)
// ============================
typedef struct {
    uint32_t sh_name;      // Section name (string tbl index)
    uint32_t sh_type;      // Section type
    uint32_t sh_flags;     // Section flags
    uint32_t sh_addr;      // Virtual address in memory
    uint32_t sh_offset;    // Offset in file
    uint32_t sh_size;      // Size of section
    uint32_t sh_link;      // Link to another section
    uint32_t sh_info;      // Misc info
    uint32_t sh_addralign; // Alignment
    uint32_t sh_entsize;   // Entry size (if table)
} Elf32_Shdr;

// ============================
// Program header types
// ============================
#define PT_NULL    0
#define PT_LOAD    1
#define PT_DYNAMIC 2
#define PT_INTERP  3
#define PT_NOTE    4
#define PT_SHLIB   5
#define PT_PHDR    6

// ============================
// Segment flags
// ============================
#define PF_X 1 // Execute
#define PF_W 2 // Write
#define PF_R 4 // Read

typedef struct process proc_t;

proc_t* create_process_from_elf(const char *filename);

#endif // ELF_H
