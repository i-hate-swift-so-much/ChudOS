#include "Userland/Tasks.h"
#include "LowLevel/Memory.h"
#include "Devices/Disk/Floppy.h"

#define TASK_COUNT 512

volatile Task TaskManager[512];

volatile int cur_pid = 0;

uint8_t Program_Buffer[0x1000];

void DumpTaskState(int pid){
    volatile Task* cur = (volatile Task*)&TaskManager[pid];
    InterruptRegisters* regs = &cur->SavedRegisters;
    printf("  State Dump of task %i\n", pid);
    printf("RIP=%x:%x RSP=%x:%x\n", regs->cs, regs->rip, regs->ss, regs->rsp);
    printf("RAX=%x RBX=%x RCX=%x RDX=%x\n", regs->rax, regs->rbx, regs->rcx, regs->rdx);
}

/**
 * @brief Gives the lower or upper bound of a page.
 * @param vaddr The base virtual address of the origin page.
 * @param page_count How many pages exist in the region.
 * @param fwd The direction of the region (true = grows up, false = grows down)
 */
uint64_t get_mem_low_high(uint64_t vaddr, uint64_t page_count, bool fwd){
    uint64_t offset = fwd ? 0x1000 : -0x1000;
    return vaddr+(page_count * offset);
}

/**
 * @brief Checks to make sure a memory access from a task is OK by checking its defined regions.
 * @param vaddr The virtual address the user is attempting to access.
 * @param pid The PID of the task trying to access the page
 */
bool mem_access_ok(uint64_t vaddr, int pid){
    volatile Task* given_task = (volatile Task*)&TaskManager[pid];
    volatile TaskMemoryDefinition m = (volatile TaskMemoryDefinition)given_task->MemoryData;
    bool in_code_region = vaddr > m.BaseVirtualAddress && vaddr < get_mem_low_high(m.BaseVirtualAddress, m.PageCount, true);
    bool in_stack_region = vaddr < (m.StackBaseVirtualAddress+0x1000) && vaddr > get_mem_low_high(m.StackBaseVirtualAddress, m.StackPageCount, false);
    bool in_kernel_stack_region = vaddr < (m.KernelStackBaseVirtualAddress+0x1000) && vaddr > get_mem_low_high(m.KernelStackBaseVirtualAddress, m.KernelStackPageCount, false);
    return in_code_region || in_stack_region || in_kernel_stack_region;
}

/**
 * @brief Used to copy n bytes of data from a user side buffer to a kernel side buffer.
 */
uint64_t copy_from_user(void* dest, const void* src, size_t n){
    if(!mem_access_ok((uint64_t)src, cur_pid)) { return n; }
    memcpy(dest, src, n);
    return 0;
}

/**
 * @brief Used to copy data from a user side null terminated string to a kernel side buffer of length count.
 */
uint64_t strcpy_from_user(void* dest, const void* src, size_t count){
    if(!mem_access_ok((uint64_t)src, cur_pid)) { return 0; }
    char* cur = (char*)src;
    char* cdest = (char*)dest;
    int i = 0;
    while(*cur != '\0' && i < count){
        *cdest++ = *cur++;
        i++;
    }
    return i;
}

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
 * @brief Frees the memory of a task by getting rid of user mappings, switching to the Kernel PML4, and unmapping the user PML4.
 */
void free_task_memory(int pid){
    uint64_t last_pml4 = PML4_Physical;
    
    // first go through the allocated pages
    TASKMGR_set_current(pid);
    volatile Task* task = (volatile Task*)&TaskManager[pid];
    uint64_t task_pml4 = task->Base_PML4;

    mem_set_cr3(task_pml4, false); // switch to user pml4
    volatile uint16_t pdpt_index, pd_index, pt_index = 0;

    // then the paging structures
    volatile uint64_t* pml4_virt;
    volatile uint64_t* pdpt_virt;
    volatile uint64_t* pd_virt;
    volatile uint64_t* pt_virt;
    pml4_virt = (volatile uint64_t*)(task_pml4 + VIRTUAL_MEMORY_BARRIER);
    pdpt_index = find_first_present(pml4_virt);

    while(pdpt_index < 256){
        pdpt_virt = (volatile uint64_t*)((pml4_virt[pdpt_index] & 0x000FFFFFFFFFF000ULL)+VIRTUAL_MEMORY_BARRIER);

        //printf("%x %x %x | A\n", pdpt_index, (uint64_t)pdpt_virt, pml4_virt[pdpt_index] & 0x000FFFFFFFFFF000ULL);
        
        pd_index = find_first_present(pdpt_virt);
        while(pd_index < 512){
            pd_virt = (volatile uint64_t*)((pdpt_virt[pd_index] & 0x000FFFFFFFFFF000ULL)+VIRTUAL_MEMORY_BARRIER);

            //printf("%x %x %x | B\n", pd_index, (uint64_t)pd_virt, pdpt_virt[pd_index] & 0x000FFFFFFFFFF000ULL);

            pt_index = find_first_present(pd_virt);
            while(pt_index < 512){
                pt_virt = (volatile uint64_t*)((pd_virt[pt_index] & 0x000FFFFFFFFFF000ULL)+VIRTUAL_MEMORY_BARRIER);

                PageDetails pt_page;
                pt_page.physical_address = pd_virt[pt_index] & 0x000FFFFFFFFFF000ULL;
                pt_page.virtual_address = (volatile uint64_t)pt_virt;
                free_page(&pt_page);

                memset(&pd_virt[pt_index], 0, 8);

                pt_index = find_first_present(pd_virt);
            }

            PageDetails pd_page;
            pd_page.physical_address = pdpt_virt[pd_index] & 0x000FFFFFFFFFF000ULL;
            pd_page.virtual_address = (uint64_t)pd_virt;
            free_page(&pd_page);

            pdpt_virt[pd_index] = 0;

            pd_index = find_first_present(pdpt_virt);
        }

        PageDetails pdpt_page;
        pdpt_page.physical_address = pml4_virt[pdpt_index] & 0x000FFFFFFFFFF000ULL;
        pdpt_page.virtual_address = (uint64_t)pdpt_virt;
        free_page(&pdpt_page);

        memset(&pml4_virt[pdpt_index], 0, 8);

        pdpt_index = find_first_present(pml4_virt);
    }
    mem_set_cr3(Kernel_PML4_Physical, false);

    memset(pml4_virt, 0, 0x1000);

    PageDetails pml4_page;
    pml4_page.physical_address = phys_addr(pml4_virt);
    pml4_page.virtual_address = (uint64_t)pml4_virt;
    free_page(&pml4_page);

    if(last_pml4 != task->Base_PML4){
        mem_set_cr3(last_pml4, false);
    }
}

void free_task_memory_by_pml4(uint64_t pml4){
    uint64_t last_pml4 = PML4_Physical;
    
    // first go through the allocated pages
    mem_set_cr3(pml4, true); // switch to user pml4
    volatile uint16_t pdpt_index, pd_index, pt_index = 0;

    // then the paging structures
    volatile uint64_t* pml4_virt;
    volatile uint64_t* pdpt_virt;
    volatile uint64_t* pd_virt;
    volatile uint64_t* pt_virt;
    pml4_virt = (volatile uint64_t*)(pml4 + VIRTUAL_MEMORY_BARRIER);
    pdpt_index = find_first_present(pml4_virt);

    while(pdpt_index < 256){
        pdpt_virt = (volatile uint64_t*)((pml4_virt[pdpt_index] & 0x000FFFFFFFFFF000ULL)+VIRTUAL_MEMORY_BARRIER);

        //printf("%x %x %x | A\n", pdpt_index, (uint64_t)pdpt_virt, pml4_virt[pdpt_index] & 0x000FFFFFFFFFF000ULL);
        
        pd_index = find_first_present(pdpt_virt);
        while(pd_index < 512){
            pd_virt = (volatile uint64_t*)((pdpt_virt[pd_index] & 0x000FFFFFFFFFF000ULL)+VIRTUAL_MEMORY_BARRIER);

            //printf("%x %x %x | B\n", pd_index, (uint64_t)pd_virt, pdpt_virt[pd_index] & 0x000FFFFFFFFFF000ULL);

            pt_index = find_first_present(pd_virt);
            while(pt_index < 512){
                pt_virt = (volatile uint64_t*)((pd_virt[pt_index] & 0x000FFFFFFFFFF000ULL)+VIRTUAL_MEMORY_BARRIER);
                
                PageDetails pt_page;
                pt_page.physical_address = pd_virt[pt_index] & 0x000FFFFFFFFFF000ULL;
                pt_page.virtual_address = (volatile uint64_t)pt_virt;
                free_page(&pt_page);

                memset(&pd_virt[pt_index], 0, 8);

                pt_index = find_first_present(pd_virt);
            }

            PageDetails pd_page;
            pd_page.physical_address = pdpt_virt[pd_index] & 0x000FFFFFFFFFF000ULL;
            pd_page.virtual_address = (uint64_t)pd_virt;
            free_page(&pd_page);

            pdpt_virt[pd_index] = 0;

            pd_index = find_first_present(pdpt_virt);
        }

        PageDetails pdpt_page;
        pdpt_page.physical_address = pml4_virt[pdpt_index] & 0x000FFFFFFFFFF000ULL;
        pdpt_page.virtual_address = (uint64_t)pdpt_virt;
        free_page(&pdpt_page);

        memset(&pml4_virt[pdpt_index], 0, 8);

        pdpt_index = find_first_present(pml4_virt);
    }

    mem_set_cr3(Kernel_PML4_Physical, false);

    memset(pml4_virt, 0, 0x1000);

    PageDetails pml4_page;
    pml4_page.physical_address = phys_addr(pml4_virt);
    pml4_page.virtual_address = (uint64_t)pml4_virt;
    free_page(&pml4_page);

    if(last_pml4 != pml4){
        mem_set_cr3(last_pml4, true);
    }else{
        sti();
    }
}

/**
 * @brief Sets up the stack for a given task, make sure to set RSP and RBP before calling this
 * @param pid The PID of the task which the stack should be made for
 * @param argv An array of strings which is placed on the stack as arguments.
 * @param argc The amount of arguments place on the stack
 */
int SetupTaskStack(int pid, char* argv[], int argc){
   /* 
    STACK LAYOUT
    +------------------+
    |   char* path[]   | // not implemented
    +------------------+
    |   char* argv[]   |
    +------------------+
    +     int argc     |
    +------------------+
    Size is not fixed
   */

    volatile Task* task = (volatile Task*)&TaskManager[pid];
    
    uint64_t last_pml4 = PML4_Physical;
    
    int last_pid = TASKMGR_get_current();
    TASKMGR_set_current(pid);

    mem_set_cr3(task->Base_PML4, true);
    char* stack = (uint8_t*)task->SavedRegisters.rsp;

    uintptr_t argv_pointers[argc + 1];
 
    uintptr_t* argv_data_start = (uintptr_t*)stack;

    for(int i = argc - 1; i >= 0; i--){
        int len = 0;
        while (argv[i][len] != '\0') len++;

        for (int j = len; j >= 0; j--) *(--stack) = argv[i][j];

        argv_pointers[i] = (uintptr_t)stack;
    }
    argv_pointers[argc] = 0; // null terminator

    for(int i = argc; i >= 0; i--){
        stack -= sizeof(uintptr_t);
        *(uint64_t*)stack = argv_pointers[i];
        //printf("%x | %x\n", (uintptr_t)stack, *(uintptr_t*)stack);
    }
    stack-=sizeof(int64_t);

    int64_t* stack_argc = (int64_t*)stack;
    *stack_argc = (int64_t)argc;

    task->SavedRegisters.rsp = (uint64_t)stack;
    //halt();

    mem_set_cr3(last_pml4, true);
    TASKMGR_set_current(last_pid);
}

/**
 * @brief Finds a free file descriptor entry in preparation for creation
 * @param PID the PID to search
 */
int FindFreeFileDescriptor(int PID){
    for(int i = 0; i < 64; i++){
        if(TaskManager[PID].Descriptors[i].used == false){
            return i;
        }
    }
}

/**
 * @brief Registers a task in the task manager, also creates the tasks memory space
 * @param base_vaddr The starting virtual address of the task
 * @param entry_point The e_entry from the elf header
 * @param page_count The amount of pages allocated to the program
 * @param maxticks How many ticks (the priority) of the program. (See Tasks.h lines 7-9)
 * @param stack_start The beginning virtual address of the programs stack.
 * @param PML4 The physical address of the new user memory created with Create_User_Memory()
 * @return Returns the PID
 */
int RegisterTask(uint64_t base_vaddr, uint64_t entry_point, uint64_t page_count, uint8_t maxticks, uint64_t stack_start, uint64_t pml4){
    int free = FindFreePID();
    if(free == TASK_COUNT+1){ return 1; }
    Task* cur_task = (Task*)&TaskManager[free];
    memset(cur_task, 0, sizeof(Task)); // zero out the task
    cur_task->MemoryData.BaseVirtualAddress = base_vaddr;
    cur_task->MemoryData.PageCount = page_count;
    cur_task->MaxTicks = maxticks;
    //cur_task->ProcessState = READY_PROCESS_STATE;
    cur_task->SavedRegisters.cs = 0x18 | 0x3; // user code segment
    cur_task->SavedRegisters.rsp = stack_start;
    cur_task->SavedRegisters.rbp = stack_start;
    cur_task->SavedRegisters.rip = entry_point;
    cur_task->SavedRegisters.ss = 0x20 | 0x3; // user data segment
    cur_task->Base_PML4 = pml4;

    cur_task->Descriptors[0].used = true;
    cur_task->Descriptors[1].used = true;

    cur_task->Exists = true;
    #ifdef ELF_SANITY
        SetTextColor(LCYAN, BLACK);
        printf("Registered a task\n\tPID = %x\n\tBASE_ADDR = %x\n\tENTRY = %x\n\tBASE_PML4 = %x\n", free, base_vaddr, entry_point+base_vaddr, pml4);
        SetTextColor(WHITE, BLACK);
    #endif
    return free;
}

/**
 * @brief Registers a task in the task manager, also creates the tasks memory space
 * @param base_vaddr The starting virtual address of the task
 * @param entry_point The e_entry from the elf header
 * @param page_count The amount of pages allocated to the program
 * @param maxticks How many ticks (the priority) of the program. (See Tasks.h lines 7-9)
 * @param stack_start The beginning virtual address of the programs stack.
 * @param PML4 The physical address of the new user memory created with Create_User_Memory()
 * @return Returns the PID
 */
int RegisterTaskStrict(uint64_t base_vaddr, uint64_t entry_point, uint64_t page_count, uint8_t maxticks, uint64_t stack_start, uint64_t pml4, int pid){
    Task* cur_task = (Task*)&TaskManager[pid];
    memset(cur_task, 0, sizeof(Task)); // zero out the task
    cur_task->MemoryData.BaseVirtualAddress = base_vaddr;
    cur_task->MemoryData.PageCount = page_count;
    cur_task->MaxTicks = maxticks;
    //cur_task->ProcessState = READY_PROCESS_STATE;
    cur_task->SavedRegisters.cs = 0x18 | 0x3; // user code segment
    cur_task->SavedRegisters.rsp = stack_start;
    cur_task->SavedRegisters.rbp = stack_start;
    cur_task->SavedRegisters.rip = entry_point;
    cur_task->SavedRegisters.ss = 0x20 | 0x3; // user data segment
    cur_task->Base_PML4 = pml4;

    cur_task->Descriptors[0].used = true;
    cur_task->Descriptors[1].used = true;

    cur_task->Exists = true;
    #ifdef ELF_SANITY
        SetTextColor(LCYAN, BLACK);
        printf("Registered a task\n\tPID = %x\n\tBASE_ADDR = %x\n\tENTRY = %x\n\tBASE_PML4 = %x\n", pid, base_vaddr, entry_point+base_vaddr, pml4);
        SetTextColor(WHITE, BLACK);
    #endif
    return pid;
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

void LoadElf_GemFS(enum GemFS_DriveIDs DriveID, uint8_t Partition, uint64_t Blocks, uint64_t Index){
    struct GemFS_Entry entry = GemFS_ReadEntry(DriveID, Partition, Index);

    uint64_t LBA = GemFS_BlockToLBA(DriveID, Partition, entry.Start);

    uint64_t BS = Drives[DriveID].Main_Entries[Partition].Block_Size;

    LoadElf(DriveID, LBA, (512*BS)*Blocks);
}

/**
    * @brief Loads a program into memory and registers it.
    * @param Drive The drive number that should be read from
    * @param LBA The starting LBA of the file
    * @param Program_Size The size (in blocks) of the program
*/
void* LoadElf(uint8_t Drive, uint64_t LBA, size_t Program_Size){
    memset(Program_Buffer, 0, 0x1000);
    
    #ifdef ELF_SANITY
        SetTextColor(LCYAN, BLACK);
        printf("Reading ELF file from drive %i LBA %i with a size of %i\n", Drive, LBA, Program_Size);
        //return NULL;
    #endif
    
    if(Drive < 0x80){
        FLOPPY_Read_LBA(Drive, LBA, (uint64_t)Program_Buffer, 8);
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
    
    uint64_t stack_base = 0x7FFF0000;

    uint64_t PrevPML4 = PML4_Physical;
    #ifdef ELF_SANITY
        SetTextColor(LCYAN, BLACK);
        printf("OLD PML4: %x NEW PML4: %x\n", PrevPML4, NewPML4);
        SetTextColor(WHITE, BLACK);
    #endif

    mem_set_cr3(NewPML4, true);

    int next_pid = FindFreePID();

    int last_pid = TASKMGR_get_current();

    TASKMGR_set_current(next_pid);

    // alloc the stack
    PageDetails stackpage;
    uint64_t free = FindNextFreePhysical();

    stackpage.physical_address = free;
    stackpage.virtual_address = stack_base & ~0xFFF;
    stackpage.flags.flags = USER_FLAGS;
    stackpage.flags.Execute_Disable = false;

    #ifdef ELF_SANITY
        printf("PHYS: %x\n", stackpage.physical_address);
        printf("VIRT: %x\n", stackpage.virtual_address);
    #endif
    TaskManager[next_pid].MemoryData.BaseVirtualAddress = 0;
    TaskManager[next_pid].Base_PML4 = NewPML4;
    //TaskManager[next_pid].Exists = true;
    TaskManager[next_pid].ProcessState = CREATION_PROCESS_STATE;

    void* stack = alloc_page(&stackpage);

    memset(stack, 0, 0x1000);

    TaskManager[next_pid].MemoryData.StackBaseVirtualAddress = stack_base;
    TaskManager[next_pid].MemoryData.StackPageCount = 1;

    uint8_t scratch_buffer[0x200];

    // loop through program headers and adjust the memsz
    uint16_t e_phnum = elf_header->e_phnum;
    for(int i = 0; i < e_phnum; i++){
        cur_header = &headers[i];
        #ifdef ELF_SANITY
            ELF_DumpProgramHeader(cur_header);
        #endif
        if(cur_header->p_type != PT_LOAD){ continue; }
        void* dest = (void*)cur_header->p_vaddr;

        if(TaskManager[next_pid].MemoryData.BaseVirtualAddress == 0){
            TaskManager[next_pid].MemoryData.BaseVirtualAddress = cur_header->p_vaddr;
        }

        uint64_t start_vaddr = cur_header->p_vaddr & ~0xFFF;
        uint64_t end_vaddr = (cur_header->p_vaddr + cur_header->p_memsz + 0xFFF) & ~0xFFF;
        uint64_t num_pages = (end_vaddr - start_vaddr) / 0x1000;

        // allocate the page needed
        PageDetails page;
        page.virtual_address = (uint64_t)dest & ~0xFFF;
        for(int i = 0; i < num_pages; i++){
            page.physical_address = FindNextFreePhysical();
            page.flags.flags = (
                0b101 | // , u/s, p
                (cur_header->p_flags & 2)
            );
            page.flags.Execute_Disable = (cur_header->p_flags & 1) ? 0 : 1;

            #ifdef ELF_SANITY
                SetTextColor(LCYAN, BLACK);
                printf("Allocating Page %x at Phys %x (D)\n", page.virtual_address, page.physical_address);
                SetTextColor(WHITE, BLACK);
            #endif

            alloc_page(&page);

            page.virtual_address += 0x1000;
            TaskManager[next_pid].MemoryData.PageCount++;
        }

        uint64_t sector_count = (cur_header->p_filesz) / 512;
        sector_count+= ((cur_header->p_filesz) % 512) > 0 ? 1 : 0;

        uint64_t proper_lba = LBA + ((cur_header->p_offset) / 512);

        #ifdef ELF_SANITY
            SetTextColor(LCYAN, BLACK);
            printf("Reading %x sectors of program data from Drive %i LBA %x to %x\n", sector_count, Drive, LBA+cur_header->p_offset, (uint64_t)dest);
            SetTextColor(WHITE, BLACK);
        #endif

        uint64_t bytes_processed = 0;
        uint64_t head_offset = cur_header->p_offset % 512;

        // in this current example, head_offset == 0
        if(head_offset > 0){
            FLOPPY_Read_LBA(Drive, proper_lba, (uint64_t)scratch_buffer, 1);
            memcpy(dest, scratch_buffer+head_offset, 0x200-head_offset);
            bytes_processed+=0x200-head_offset;
            proper_lba++;
        }
        uint64_t remaining_bytes = cur_header->p_filesz - bytes_processed;
        if(remaining_bytes > 0){
            uint64_t sectors_to_read = (remaining_bytes + 511) / 512;

            FLOPPY_Read_LBA(Drive, proper_lba, (uint64_t)dest+bytes_processed, sectors_to_read);
        }
    }
    #ifdef ELF_SANITY
        printf("NUM: %x\n", e_phnum);
    #endif
    // create the kernel stack
    TaskManager[next_pid].MemoryData.KernelStackPageCount = 5;
    malloc(KernelTask);
    malloc(KernelTask);
    malloc(KernelTask);
    malloc(KernelTask);
    uint64_t kernel_stack = (uint64_t)malloc(KernelTask);
    TaskManager[next_pid].MemoryData.KernelStackBaseVirtualAddress = kernel_stack;

    TASKMGR_set_current(last_pid);

    RegisterTask(TaskManager[next_pid].MemoryData.BaseVirtualAddress, elf_header->e_entry, TaskManager[next_pid].MemoryData.PageCount, USER_PRIORITY, stack_base+4080, NewPML4);

    TaskManager[next_pid].MemoryData.StackBaseVirtualAddress = stack_base;
    TaskManager[next_pid].MemoryData.StackPageCount = 1;
    TaskManager[next_pid].MemoryData.KernelStackPageCount = 5;
    TaskManager[next_pid].MemoryData.KernelStackBaseVirtualAddress = kernel_stack;

    // set up the TSS
    memcpy(&TaskManager[next_pid].UserTSS, &KernelTask->UserTSS, sizeof(struct TSS));
    TaskManager[next_pid].UserTSS.rsp0 = kernel_stack;

    char* argv[] = {"test arg"};

    SetupTaskStack(next_pid, argv, 1); // setup stack

    TaskManager[next_pid].ProcessState = READY_PROCESS_STATE;

    mem_set_cr3(PrevPML4, true);

    return (void*)TaskManager[next_pid].MemoryData.BaseVirtualAddress;
}

/**
    * @brief Loads a program into memory and registers it.
    * @param Drive The drive number that should be read from
    * @param LBA The starting LBA of the file
    * @param Program_Size The size (in blocks) of the program
*/
void* LoadElfStrict(uint8_t Drive, uint64_t LBA, size_t Program_Size, int pid, uint64_t NewPML4, char* argv[], int argc){
    memset(Program_Buffer, 0, 0x1000);

    #ifdef ELF_SANITY
        SetTextColor(LCYAN, BLACK);
        printf("Reading ELF file from drive %i LBA %i with a size of %i\n", Drive, LBA, Program_Size);
        //return NULL;
    #endif

    if(Drive < 0x80){
        FLOPPY_Read_LBA(Drive, LBA, (uint64_t)Program_Buffer, 8);
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
    
    uint64_t stack_base = 0x7FFF0000;

    uint64_t PrevPML4 = PML4_Physical;
    #ifdef ELF_SANITY
        SetTextColor(LCYAN, BLACK);
        printf("OLD PML4: %x NEW PML4: %x\n", PrevPML4, NewPML4);
        SetTextColor(WHITE, BLACK);
    #endif

    mem_set_cr3(NewPML4, true);

    int next_pid = pid;

    int last_pid = TASKMGR_get_current();

    TASKMGR_set_current(next_pid);

    // alloc the stack
    PageDetails stackpage;
    uint64_t free = FindNextFreePhysical();

    stackpage.physical_address = free;
    stackpage.virtual_address = stack_base & ~0xFFF;
    stackpage.flags.flags = USER_FLAGS;
    stackpage.flags.Execute_Disable = false;

    #ifdef ELF_SANITY
        printf("PHYS: %x\n", stackpage.physical_address);
        printf("VIRT: %x\n", stackpage.virtual_address);
    #endif
    TaskManager[next_pid].MemoryData.BaseVirtualAddress = 0;
    TaskManager[next_pid].Base_PML4 = NewPML4;
    //TaskManager[next_pid].Exists = true;
    TaskManager[next_pid].ProcessState = CREATION_PROCESS_STATE;

    void* stack = alloc_page(&stackpage);

    memset(stack, 0, 0x1000);

    TaskManager[next_pid].MemoryData.StackBaseVirtualAddress = stack_base;
    TaskManager[next_pid].MemoryData.StackPageCount = 1;

    uint8_t scratch_buffer[0x200];

    // loop through program headers and adjust the memsz
    uint16_t e_phnum = elf_header->e_phnum;
    for(int i = 0; i < e_phnum; i++){
        cur_header = &headers[i];
        #ifdef ELF_SANITY
            ELF_DumpProgramHeader(cur_header);
        #endif
        if(cur_header->p_type != PT_LOAD){ continue; }
        void* dest = (void*)cur_header->p_vaddr;

        if(TaskManager[next_pid].MemoryData.BaseVirtualAddress == 0){
            TaskManager[next_pid].MemoryData.BaseVirtualAddress = cur_header->p_vaddr;
        }

        uint64_t start_vaddr = cur_header->p_vaddr & ~0xFFF;
        uint64_t end_vaddr = (cur_header->p_vaddr + cur_header->p_memsz + 0xFFF) & ~0xFFF;
        uint64_t num_pages = (end_vaddr - start_vaddr) / 0x1000;

        // allocate the page needed
        PageDetails page;
        page.virtual_address = (uint64_t)dest & ~0xFFF;
        for(int i = 0; i < num_pages; i++){
            page.physical_address = FindNextFreePhysical();
            page.flags.flags = (
                0b101 | // , u/s, p
                (cur_header->p_flags & 2)
            );
            page.flags.Execute_Disable = (cur_header->p_flags & 1) ? 0 : 1;

            #ifdef ELF_SANITY
                SetTextColor(LCYAN, BLACK);
                printf("Allocating Page %x at Phys %x (D)\n", page.virtual_address, page.physical_address);
                SetTextColor(WHITE, BLACK);
            #endif

            alloc_page(&page);

            page.virtual_address += 0x1000;
            TaskManager[next_pid].MemoryData.PageCount++;
        }

        uint64_t sector_count = (cur_header->p_filesz) / 512;
        sector_count+= ((cur_header->p_filesz) % 512) > 0 ? 1 : 0;

        uint64_t proper_lba = LBA + ((cur_header->p_offset) / 512);

        #ifdef ELF_SANITY
            SetTextColor(LCYAN, BLACK);
            printf("Reading %x sectors of program data from Drive %i LBA %x to %x\n", sector_count, Drive, LBA+cur_header->p_offset, (uint64_t)dest);
            SetTextColor(WHITE, BLACK);
        #endif

        uint64_t bytes_processed = 0;
        uint64_t head_offset = cur_header->p_offset % 512;

        // in this current example, head_offset == 0
        if(head_offset > 0){
            FLOPPY_Read_LBA(Drive, proper_lba, (uint64_t)scratch_buffer, 1);
            memcpy(dest, scratch_buffer+head_offset, 0x200-head_offset);
            bytes_processed+=0x200-head_offset;
            proper_lba++;
        }
        uint64_t remaining_bytes = cur_header->p_filesz - bytes_processed;
        if(remaining_bytes > 0){
            uint64_t sectors_to_read = (remaining_bytes + 511) / 512;

            FLOPPY_Read_LBA(Drive, proper_lba, (uint64_t)dest+bytes_processed, sectors_to_read);
        }
    }
    #ifdef ELF_SANITY
        printf("NUM: %x\n", e_phnum);
    #endif

    TaskManager[next_pid].MemoryData.KernelStackPageCount = 5;
    malloc(KernelTask);
    malloc(KernelTask);
    malloc(KernelTask);
    malloc(KernelTask);
    uint64_t kernel_stack = (uint64_t)malloc(KernelTask);
    TaskManager[next_pid].MemoryData.KernelStackBaseVirtualAddress = kernel_stack;

    TASKMGR_set_current(last_pid);

    RegisterTaskStrict(TaskManager[next_pid].MemoryData.BaseVirtualAddress, elf_header->e_entry, TaskManager[next_pid].MemoryData.PageCount, USER_PRIORITY, stack_base+4080, NewPML4, pid);
    
    TaskManager[next_pid].MemoryData.StackBaseVirtualAddress = stack_base;
    TaskManager[next_pid].MemoryData.StackPageCount = 1;
    TaskManager[next_pid].MemoryData.KernelStackPageCount = 5;
    TaskManager[next_pid].MemoryData.KernelStackBaseVirtualAddress = kernel_stack;

    memcpy(&TaskManager[next_pid].UserTSS, &KernelTask->UserTSS, sizeof(struct TSS));
    TaskManager[next_pid].UserTSS.rsp0 = kernel_stack;

    SetupTaskStack(next_pid, argv, argc);

    TaskManager[next_pid].ProcessState = READY_PROCESS_STATE;

    mem_set_cr3(PrevPML4, true);

    return (void*)TaskManager[next_pid].MemoryData.BaseVirtualAddress;
}

void LoadElfStrict_GemFS(enum GemFS_DriveIDs DriveID, uint8_t Partition, uint64_t Blocks, uint64_t Index, int pid, uint64_t NewPML4, char* argv[], int argc){
    struct GemFS_Entry entry = GemFS_ReadEntry(DriveID, Partition, Index);

    uint64_t LBA = GemFS_BlockToLBA(DriveID, Partition, entry.Start);

    uint64_t BS = Drives[DriveID].Main_Entries[Partition].Block_Size;

    LoadElfStrict(DriveID, LBA, (Blocks*BS)*512, pid, NewPML4, argv, argc);
}

int TASKMGR_get_current(){
    return cur_pid;
}

void TASKMGR_set_current(int pid){
    cur_pid = pid;
}

void SignalOwner(volatile Task* Origin, enum ChildWaitReasons Reason, int ret){
    //printf("SIGNALLING OWNER (PID %x)\n", Origin->Owner_PID);
    if(Origin->Owner_PID == 0){ return; }
    volatile Task* owner = (volatile Task*)&TaskManager[Origin->Owner_PID];

    if(owner->ProcessState == WAITING_PROCESS_STATE && owner->WaitingReason == WAITING_REASON_CHILD && owner->ChildWaitingReason == Reason){
        owner->SavedRegisters.rax = ret;
        owner->ProcessState = READY_PROCESS_STATE;
        owner->WaitingReason = WAITING_REASON_NULL;
    }
}