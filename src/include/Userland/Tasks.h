#pragma once
#include "LowLevel/Memory.h"
#include "Libraries/Math.h"
#include "Libraries/std.h"
#include "filesys/gemfs.h"


// Defines how many cycles each level of priority can use
#define SUDO_PRIORITY 40
#define USER_PRIORITY 20
#define LOW_PRIORITY 10
#define DAEMON_PRIORITY 5

// Defines the type of states a process can be in
// used when setting Task.ProcessState
#define NULL_PROCESS_STATE 0x00 // this is used because when the list of processes is enabled, all entries are just zeros. If this was anything else the scheduler would read it as valid even though it isn't initialized
#define CREATION_PROCESS_STATE 0x01 // process is still being created and should not be ran.
#define READY_PROCESS_STATE 0x02 // process is ready to be ran by the cpu
#define WAITING_PROCESS_STATE 0x03 // process is waiting in one of the waiting queues
#define KILL_PROCESS_STATE 0x04 // the process should be terminated

#define WAITING_REASON_NULL 0x00 // Used when the process isn't waiting. Filler
#define WAITING_REASON_INPUT 0x01 // Used when the process is waiting for keyboard input through read(STD_IN_FD, &foo1, foo2);
#define WAITING_REASON_SLEEP 0x02 // Used when the process is sleeping for a specified amount of ticks (see struct Tasks_s.SleepCycle and struct Tasks_s.RequestedSleepCycle)
#define WAITING_REASON_CHILD 0x03 // Used when the process is waiting for its child to do something

// values for e_type
#define ET_NONE 0x00
#define ET_REL 0x01
#define ET_EXEC 0x02 // This is means it's fixed address (must be loaded at specific VA)
#define ET_DYN 0x03 // THIS is the preferred type, it means it can be relocated anywhere
#define ET_CORE 0x04

// values for e_machine
#define EM_x86 0x03
#define EM_x86_64 0x3E

// values for p_type
#define PT_LOAD 0x01

// values for p_flags
#define PF_X 0x01
#define PF_W 0x02
#define PF_R 0x03

struct elf_identifiers{
    uint8_t magic[4];
    uint8_t EI_CLASS;
    uint8_t EI_DATA;
    uint8_t EI_VERSION;
    uint8_t EI_OSABI;
    // 8 bytes of padding
    uint8_t pad[8];
};

struct ElfHeader64_s{
    struct elf_identifiers e_ident;
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
};

typedef struct ElfHeader64_s ElfHeader64;

struct ProgramHeader64_s{
    uint32_t p_type; // segment type
    uint32_t p_flags; // segment-dependant flags
    uint64_t p_offset; // offset of the segment in the file image
    uint64_t p_vaddr; // intended virtual address
    uint64_t p_paddr; // unused in modern systems
    uint64_t p_filesz; // size of the segment in the file 
    uint64_t p_memsz; // size of the segment in memory
    uint64_t p_align;
};

typedef struct ProgramHeader64_s ProgramHeader64;

uint64_t get_mem_low_high(uint64_t vaddr, uint64_t page_count, bool fwd);
bool mem_access_ok(uint64_t vaddr, int pid);
uint64_t copy_from_user(void* dest, const void* src, size_t n);
uint64_t strcpy_from_user(void* dest, const void* src, size_t count);


void SignalOwner(volatile Task* Origin, enum ChildWaitReasons Reason, int ret);
void DumpTaskState(int pid);

int RegisterTask(uint64_t base_vaddr, uint64_t entry_point, uint64_t page_count, uint8_t maxticks, uint64_t stack_start, uint64_t pml4);
int RegisterTaskStrict(uint64_t base_vaddr, uint64_t entry_point, uint64_t page_count, uint8_t maxticks, uint64_t stack_start, uint64_t pml4, int pid);

/**
    * @brief Loads a program into memory and registers it.
    * @param Drive The drive number that should be read from
    * @param LBA The starting LBA of the file
    * @param Program_Size The size (in bytes) of the program
*/
void* LoadElf(uint8_t Drive, uint64_t LBA, size_t Program_Size);
void* LoadElfStrict(uint8_t Drive, uint64_t LBA, size_t Program_Size, int pid, uint64_t NewPML4, char* argv[], int argc);
int SetupTaskStack(int pid, char* argv[], int argc);

void free_task_memory(int pid);
void free_task_memory_by_pml4(uint64_t pml4);
int TASKMGR_get_current();
void TASKMGR_set_current(int pid);
int FindFreeFileDescriptor(int PID);

void KillTask(int PID);

void LoadElf_GemFS(enum GemFS_DriveIDs DriveID, uint8_t Partition, uint64_t Blocks, uint64_t Index);
void LoadElfStrict_GemFS(enum GemFS_DriveIDs DriveID, uint8_t Partition, uint64_t Blocks, uint64_t Index, int pid, uint64_t NewPML4, char* argv[], int argc);

void TaskStack_Push(int pid, uint64_t value, bool setcr3);
uint64_t TaskStack_Pop(int pid, bool setcr3);