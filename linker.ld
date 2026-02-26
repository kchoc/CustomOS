OUTPUT_FORMAT("elf32-i386")
ENTRY(btext)

KERNEL_VIRT = 0xC0000000;
KERNEL_PHYS = 0x00200000;

SECTIONS
{
    . = KERNEL_VIRT;
    _kernel_start = .;

    .text : AT(KERNEL_PHYS)
    {
        btext = .;
        *(.multiboot)
        *(.text*)
        *(.rodata*)
        *(.data*)
    }

    .bss ALIGN(4096) :
    {
        __bss_start = .;
        *(COMMON)
        *(.bss*)
        __bss_end = .;
    }

    _kernel_end = .;

    /DISCARD/ :
    {
        *(.eh_frame)
        *(.eh_frame_hdr)
        *(.comment)
        *(.note*)
    }
}
