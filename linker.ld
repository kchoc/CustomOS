OUTPUT_FORMAT("elf32-i386")
ENTRY (_start)

SECTIONS
{
    /* ------------------------------------------------------------------
     * 1) Multiboot header and small bootstrap data must be at start of FILE
     *    so the bootloader can find them (within first 8 KiB of file).
     * ------------------------------------------------------------------*/
    .multiboot  : {
        KEEP(*(.multiboot))
        KEEP(*(.multiboot.*))
        KEEP(*(.multiboot.data))
        KEEP(*(.multiboot.text))
    }

    /* ------------------------------------------------------------------
     * 2) Trampoline: put at physical 0x9000 (SIPI vector 0x09).
     * ------------------------------------------------------------------*/
    .trampoline 0x00009000 : AT(0x00009000) {
        KEEP(*(.trampoline16))
        KEEP(*(.trampoline32))
        KEEP(*(.trampoline_gdt))
        PROVIDE(trampoline_start = .);
        . = ALIGN(4096);
    }

    /* ------------------------------------------------------------------
     * 3) Kernel proper: load at physical 1 MiB, run at virtual 0xC0100000.
     * ------------------------------------------------------------------*/
    . = 0x00100000;      /* VMA: where kernel will run (virtual) */

    _kernel_start = .;   /* VMA start symbol */

    /* Now add an offset so the file-backed section addresses are computed as
       virtual - 0xC0000000, which seems to be your existing approach. */
    . += 0xC0000000;

    .text ALIGN(4K) : AT(ADDR(.text) - 0xC0000000)
    {
        *(.text)
        *(.text.*)
    }

    .rodata ALIGN(4K) : AT(ADDR(.rodata) - 0xC0000000)
    {
        *(.rodata)
        *(.rodata.*)
    }

    .data ALIGN(4K) : AT(ADDR(.data) - 0xC0000000)
    {
        *(.data)
        *(.data.*)
    }

    .bss ALIGN(4K) : AT(ADDR(.bss) - 0xC0000000)
    {
        *(COMMON)
        *(.bss)
        *(.bootstrap_stack)
    }

    _kernel_end = .;
}
