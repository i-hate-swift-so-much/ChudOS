OUTPUT("elf_i686")
ENTRY(_start)

SECTIONS
{
    . = 0x000070000;
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