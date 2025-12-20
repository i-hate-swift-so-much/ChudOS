#pragma once
#include "Memory.h"
#include "Math.h"
#include "std.h"

// Defines how many cycles each level of priority can use
#define SUDO_PRIORITY 40
#define USER_PRIORITY 20
#define LOW_PRIORITY 10

// Defines the type of states a process can be in
// used when setting Task.ProcessState
#define NULL_PROCESS_STATE 0x00 // this is used because when the list of processes is enabled, all entries are just zeros. If this was anything else the scheduler would read it as valid even though it isn't initialized
#define NEW_PROCESS_STATE 0x01 // process was just created and it's memory is being allocated
#define READY_PROCESS_STATE 0x02 // process is ready to be ran by the cpu
#define RUNNING_PROCESS_STATE 0x03 // process is currently being run by the cpu. Only one process can be running at a time, if multiple are then a kernel panic will be triggered
#define WAITING_PROCESS_STATE 0x04 // process is waiting in one of the waiting queues

// values for e_type
#define ET_NONE 0x00
#define ET_REL 0x01
#define ET_EXEC 0x02 // This is means it's fixed address (must be loaded at specific VA)
#define ET_DYN 0x03 // THIS is the preferred type, it means it can be relocated anywhere
#define ET_CORE 0x04

// values for e_machine
#define EM_x86 0x03 // only value that will be accepted

// values for p_type
#define PT_LOAD 0x01

// values for p_flags
#define PF_X 0x01
#define PF_W 0x02
#define PF_R 0x03

/*
    NOTE: certain header values must be modified using patchelf.
    Elf header values that must be modified and what they should be set to.
        e_ident[EI_OSABI] = 0x13
        e_ident[EI_ABIVERSION] = 0xFF
    If these aren't changed, ChudOS will refuse to run the program.
    (EI_OSABIx13 is undefined, EI_ABIVERSION is a second check)
*/

struct elf_identifiers{
    uint8_t magic[4];
    uint8_t EI_CLASS;
    uint8_t EI_DATA;
    uint8_t EI_VERSON;
    uint8_t EI_OSABI;
    uint8_t EI_ABIVERSION;
    // reserved
    uint8_t pad[7] = {};
}__attribute__((packed));

struct ElfHeader64{
    elf_identifiers e_ident;
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint64_t e_entry; // where the first real instruction is
    uint64_t e_phoff; // program header offset
    uint64_t e_shoff; // section header offset
    uint32_t e_flags; // flags, always 0 for x86/x86-64
    uint16_t e_ehsize; // size of the Elf Header
    uint16_t e_phentsize; // size of a program header entry
    uint16_t e_phnum; // number of entries in the program header table
    uint16_t e_shentsize; // size of a section header entry, 0x40 for 64 bit programs
    uint16_t e_shnum; // number of entries in the section header table
    uint16_t e_shstrndx; // index of the section header table that contains the section names
}__attribute__((packed));

struct ProgramHeader{
    uint32_t p_type; // segment type
    uint32_t p_flags; // segment-dependant flags
    uint64_t p_offset; // offset of the segment in the file image
    uint64_t p_vaddr; // intended virtual address,
    uint64_t p_paddr; // unused in modern systems
    uint64_t p_filesz; // size of the segment in the file 
    uint64_t p_memsz; // size of the segment in memory
    uint64_t p_align;
}__attribute__((packed));