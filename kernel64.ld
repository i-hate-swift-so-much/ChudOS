OUTPUT_FORMAT("elf64-x86-64")
OUTPUT_ARCH(i386:x86-64)
ENTRY(_start)

SECTIONS
{
    . = 0x000080000;
    .text : { 
        *kernel_entry.o(.text)
        *(.text*) 
    }
    .rodata : { *(.rodata*) }
    .data : { *(.data*) }
    .bss : { *(COMMON) *(.bss*) }

    /DISCARD/ : 
    {
        *(.comment)
        *(.eh_frame)
        *(.note.GNU-stack)
    }
}