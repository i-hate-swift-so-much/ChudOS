#include "Tasks.h"
#include "Memory.h"

Task TaskManager[512];

/*
    NOTE: certain header values must be modified using patchelf.
    Elf header values that must be modified and what they should be set to.
        e_ident[EI_OSABI] = 0x13
        e_ident[EI_ABIVERSION] = 0xFF
    If these aren't changed, ChudOS will refuse to run the program.
    (EI_OSABIx13 is undefined, so I use it for this kernel, EI_ABIVERSION is a second check)
*/
/*
    const void* RawFile should point to an unparsed elf file that has been completely loaded into memory from the HDD
*/
void* LoadElf(const void* RawFile){
    ElfHeader64 elf_header;
    memcpy(&elf_header, RawFile, 64);

    uint64_t base = rand64();

    if(
        elf_header.e_type != ET_DYN || 
        elf_header.e_machine != EM_x86 || 
        elf_header.e_ident.EI_OSABI != 0x13 || 
        elf_header.e_ident.EI_ABIVERSION != 0xFF ||
        elf_header.e_phentsize != 0x40
    ){ return nullptr; }

    uint16_t e_phnum = elf_header.e_phnum;
    for(int i = 0; i < e_phnum; i++){

    }
}