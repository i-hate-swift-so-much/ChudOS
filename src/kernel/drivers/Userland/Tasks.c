#include "Userland/Tasks.h"
#include "LowLevel/Memory.h"
#include "Devices/Disk/Floppy.h"

#define TASK_COUNT 512

Task TaskManager[512];

int cur_pid = 0;

uint8_t Program_Buffer[0x40000];

void KillTask(int PID){
    TaskManager[PID].ProcessState = KILL_PROCESS_STATE;
}

int FindFreePID(){
    for(int i = 0; i < TASK_COUNT; i++){
        if(TaskManager[i].Exists == false){
            return i;
        }
    }
    return TASK_COUNT+1;
}

/**
 * @brief Sets up the stack for a given task, make sure to set RSP and RSP before calling this
 * @param task The task to set up the stack for
 */
int SetupTaskStack(Task* task, uint64_t* args){
    
}

/**
 * @brief Registers a task in the task manager, also creates the tasks memory space
 * @param base_vaddr The starting virtual address of the task
 * @param entry_point The e_entry from the elf header
 * @param page_count The amount of pages allocated to the program
 * @param maxticks How many ticks (the priority) of the program. (See Tasks.h lines 7-9)
 * @param stack_start The beginning virtual address of the programs stack.
 * @param PML4 The physical address of the new user memory created with Create_User_Memory()
 * @return 0 = Success
 * @return 1 = No free PID's
 * @return 2 = Couldn't create stack
 */
int RegisterTask(uint64_t base_vaddr, uint64_t entry_point, uint64_t page_count, uint8_t maxticks, uint64_t stack_start, uint64_t pml4){
    int free = FindFreePID();
    if(free == TASK_COUNT+1){ return 1; }
    Task* cur_task = (Task*)&TaskManager[free];
    memset(cur_task, 0, sizeof(Task)); // zero out the task
    cur_task->MemoryData.BaseVirtualAddress = base_vaddr;
    cur_task->MemoryData.PageCount = page_count;
    cur_task->MaxTicks = maxticks;
    cur_task->ProcessState = READY_PROCESS_STATE;
    cur_task->SavedRegisters.cs = 0x18 | 0x3; // user code segment
    cur_task->SavedRegisters.rsp = stack_start;
    cur_task->SavedRegisters.rbp = stack_start;
    cur_task->SavedRegisters.rip = entry_point+base_vaddr;
    cur_task->SavedRegisters.ss = 0x20 | 0x3; // user data segment
    cur_task->Base_PML4 = pml4;
    cur_task->Exists = true;
    #ifdef ELF_SANITY
        SetTextColor(LCYAN, BLACK);
        printf("Registered a task\n\tPID = %x\n\tBASE_ADDR = %x\n\tENTRY = %x\n\tBASE_PML4 = %x\n", free, base_vaddr, entry_point+base_vaddr, pml4);
        SetTextColor(WHITE, BLACK);
    #endif
    return 0;
}

/**
 * @brief Dumps a formatted 64 bit ELF program header with value names
 * 
 */
void ELF_DumpProgramHeader(ProgramHeader64* header){
    SetTextColor(LCYAN, BLACK);
    printf("Dumping ELF 64 bit Program Header\nDATA:\n");
    printf("\tTYPE: %x\n", header->p_type);
    printf("\tFLAGS: %x\n", header->p_flags);
    printf("\tOFFSET: %x\n", header->p_offset);
    printf("\tVADDR: %x\n", header->p_vaddr);
    printf("\tPADDR: %x\n", header->p_paddr);
    printf("\tFILE SIZE: %x\n", header->p_filesz);
    printf("\tMEM SIZE: %x\n", header->p_memsz);
    printf("\tALIGN: %x\n", header->p_align);
    SetTextColor(WHITE, BLACK);
}

/**
 * @brief Dumps a formatted 64 bit ELF header with value names (takes up a LOT!!! of screen space)
 * @param header pointer to a 64 bit Elf header struct
 */
void ELF_DumpHeader(ElfHeader64* header){
    SetTextColor(LCYAN, BLACK);
    printf("Dumping ELF 64 bit Header\nIDENTIFIER:\n");
    printf("\tMAGIC: %x\n", header->e_ident.magic[0]);
    printf("\tSTR: %c%c%c\n", header->e_ident.magic[1],header->e_ident.magic[2],header->e_ident.magic[3]);
    printf("\tCLASS: %x\n", header->e_ident.EI_CLASS);
    printf("\tDATA: %x\n", header->e_ident.EI_DATA);
    printf("\tVERSION: %x\n", header->e_ident.EI_VERSION);
    printf("\tOS_ABI: %x\n", header->e_ident.EI_OSABI);
    printf("DATA:\n");
    printf("\tTYPE: %x\n", header->e_type);
    printf("\tMACHINE: %x\n", header->e_machine);
    printf("\tVERSION: %x\n", header->e_version);
    printf("\tENTRY: %x\n", header->e_entry);
    printf("\tPH_OFF: %x\n", header->e_phoff);
    printf("\tFLAGS: %x\n", header->e_flags);
    printf("\tHEADER SIZE: %x\n", header->e_ehsize);
    printf("\tPROGRAM HEADER SIZE: %x\n", header->e_phentsize);
    printf("\tPROGRAM HEADER COUNT: %x\n", header->e_phnum);
    SetTextColor(WHITE, BLACK);
}

/**
    * @brief Loads a program into memory and registers it.
    * @param Drive The drive number that should be read from
    * @param LBA The starting LBA of the file
    * @param Program_Size The size (in blocks) of the program
*/
void* LoadElf(uint8_t Drive, uint64_t LBA, size_t Program_Size){
    memset(Program_Buffer, 0, 0x40000);
    
    #ifdef ELF_SANITY
        SetTextColor(LCYAN, BLACK);
        printf("Reading ELF file from drive %i LBA %i with a size of %i\n", Drive, LBA, Program_Size);
    #endif
    
    if(Drive < 0x80){
        FLOPPY_Read_LBA(Drive, LBA, (uint64_t)Program_Buffer, Program_Size);
    }

    ElfHeader64* elf_header = (ElfHeader64*)Program_Buffer;
    
    if(
        elf_header->e_type != ET_EXEC || 
        elf_header->e_machine != EM_x86_64 || 
        elf_header->e_phentsize != 0x38
    ){ 
        #ifdef ELF_SANITY
            SetTextColor(LCYAN, BLACK);
            printf("E_TYPE: %i\nEXPECTED: %i\nE_MACHINE: %i\nEXPECTED: %i\nE_PHENTSIZE: %x\nEXPECTED: %x\n", elf_header->e_type, ET_EXEC, elf_header->e_machine, EM_x86_64, elf_header->e_phentsize, 0x38);
        #endif
        return NULL;
    }

    #ifdef ELF_SANITY
        SetTextColor(LCYAN, BLACK);
        printf("Real program\n");
        ELF_DumpHeader(elf_header);
    #endif

    // get the header table at the offset defined in the header
    ProgramHeader64* headers = (ProgramHeader64*)(Program_Buffer + elf_header->e_phoff);

    ProgramHeader64* cur_header;

    uint64_t NewPML4 = Create_User_Memory();
    
    uint64_t base = 0x400000;
    uint64_t stack_base = base;
    #ifdef ELF_SANITY
        printf("BASE: %x\n", base);
    #endif

    uint64_t PrevPML4 = PML4_Physical;
    
    mem_set_cr3(NewPML4);

    // alloc the stack
    PageDetails stackpage;
    PageEntries free = FindNextFreePhysical();
    PagePermissions stackperms;
    stackperms.flags = USER_FLAGS;
    stackperms.Execute_Disable = true;

    stackpage.physical_address = CalculatePageAddress(&free);
    stackpage.virtual_address = base & ~0xFFF;
    stackpage.flags = stackperms;

    #ifdef ELF_SANITY
        printf("PHYS: %x\n", stackpage.physical_address);
        printf("VIRT: %x\n", stackpage.virtual_address);
    #endif

    alloc_page(&stackpage);

    memset((void*)base, 0, 0x1000);

    base+= 0x1000; // skip the stack

    uint64_t start_base = base;

    uint64_t memsz = 0;

    uint64_t start_offset = 0;
        
    // loop through program headers and adjust the memsz
    uint16_t e_phnum = elf_header->e_phnum;
    for(int i = 0; i < e_phnum; i++){
        #ifdef DEBUG
            ELF_DumpProgramHeader(cur_header);
        #endif
        cur_header = &headers[i];
        memsz += cur_header->p_memsz;
        if(i == 0){ start_offset = cur_header->p_offset; }
    }
    #ifdef ELF_SANITY
        printf("NUM: %x\nMEMSZ: %x\n", e_phnum, memsz);
    #endif

    PagePermissions flags;
    flags.flags = USER_FLAGS;
    flags.Execute_Disable = false;

    PageDetails to_alloc;
    to_alloc.flags.flags = USER_FLAGS;
    to_alloc.flags.Execute_Disable = false;

    for(int i = 0; i < memsz/0x1000; i++){
        PageEntries free = FindNextFreePhysical();
        to_alloc.physical_address = CalculatePageAddress(&free);
        to_alloc.virtual_address = base;

        #ifdef ELF_SANITY
            SetTextColor(LCYAN, BLACK);
            printf("User Page Address: %x (DIV)\n", base);
            SetTextColor(WHITE, BLACK);
        #endif

        alloc_page(&to_alloc);

        base+=0x1000;
    }

    if(memsz % 0x1000 > 0){
        PageEntries free = FindNextFreePhysical();
        to_alloc.physical_address = CalculatePageAddress(&free);
        to_alloc.virtual_address = base;

        #ifdef ELF_SANITY
            SetTextColor(LCYAN, BLACK);
            printf("User Page Address: %x (MOD)\n", base);
        #endif

        alloc_page(&to_alloc);
    }

    memcpy((void*)(start_base), &Program_Buffer[start_offset], memsz);

    #ifdef ELF_SANITY
        printf("Copied\n");
        SetTextColor(WHITE, BLACK);
    #endif

    RegisterTask(start_base, elf_header->e_entry, 5, USER_PRIORITY, stack_base+4080, NewPML4);

    mem_set_cr3(PrevPML4);

    return (void*)base;
}

int TASKMGR_get_current(){
    return cur_pid;
}

void TASKMGR_set_current(int pid){
    cur_pid = pid;
}