OUTPUT_FORMAT("elf64-x86-64")
OUTPUT_ARCH(i386:x86-64)
ENTRY(_start)

SECTIONS
{
    . = 0xffff800000100000;
    .text : ALIGN(4096) { 
        *kernel_entry.o(.text)
        *(.text*) 
    }
    .rodata : ALIGN(4096) { *(.rodata*) }
    .data : ALIGN(4096) { *(.data*) }
    .bss : ALIGN(4096) { *(COMMON) *(.bss*) }


    /DISCARD/ : 
    {
        *(.comment)
        *(.eh_frame)
        *(.note.GNU-stack)
    }
}