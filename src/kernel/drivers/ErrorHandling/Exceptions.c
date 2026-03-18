#include "ErrorHandling/Exceptions.h"

uint8_t CoW_bounce_buffer[0x1000];

InterruptRegisters ErrToInt(InterruptRegistersError regs){
    InterruptRegisters ret;

    ret.int_no = regs.int_no;
    ret.cr2 = regs.cr2;
    ret.r15 = regs.r15;
    ret.r14 = regs.r14;
    ret.r13 = regs.r13;
    ret.r12 = regs.r12;
    ret.r11 = regs.r11;
    ret.r10 = regs.r10;
    ret.r9 = regs.r9;
    ret.r8 = regs.r8;
    ret.rdi = regs.rdi;
    ret.rsi = regs.rsi;
    ret.rbp = regs.rbp;
    ret.rbx = regs.rbx;
    ret.rdx = regs.rdx;
    ret.rcx = regs.rcx;
    ret.rax = regs.rax;
    ret.rip = regs.rip;
    ret.cs = regs.cs;
    ret.rflags = regs.rflags;
    ret.rsp = regs.rsp;
    ret.ss = regs.ss;
    return ret;
}

void HandlePageFault(InterruptRegistersError* regs){
    uint64_t newPhysical = FindNextFreePhysical();
    if(false){
        printf("FAULT addr:%x\n", regs->cr2);
        printf("Process Dump PID=%i\n", TASKMGR_get_current());
        printf("RAX=%x\nRBX=%x\nRCX=%x\nRDX=%x\n", regs->rax, regs->rbx, regs->rcx, regs->rdx);
        printf("RIP=%x\n", regs->rip);
    }
    uint64_t virtual_address = regs->cr2;

    if(virtual_address < 0x1000){
        asm volatile(
            "cli\n"
            "hlt\n"
        );
    }

    uint64_t CR3;
    asm volatile(
        "movq %%cr3, %%rax\n"
        "movq %%rax, %0\n"
        : "=r" (CR3) : :
    );

    virtual_address = virtual_address & ~0xFFF;

    Task* cur_task = &TaskManager[TASKMGR_get_current()];
    cur_task->MemoryData.PageCount+=1;

    if(virtual_address < VIRTUAL_MEMORY_BARRIER){
        if(regs->rip < VIRTUAL_MEMORY_BARRIER){
            // determine if the entry is CoW available.
            PageEntries entries = ExtractPageEntries(virtual_address);

            uint64_t* page_data = CalculatePagePhysicalEntryAddress(&entries);
            // if CoW bit of the page entry is set, and the P and W bits of the error is set, then it should copy.
            if(*page_data & 1<<9 && regs->error_code & 0b11 == 0b11){
                // find a free physical, if it doesn't exist kill the task
                uint64_t phys = FindNextFreePhysical();
                if(phys > total_mem){
                    KillTask(TASKMGR_get_current());
                    InterruptRegisters regs_i = ErrToInt(*regs);
                    ForceSwitch(&regs_i);
                    return;
                }
                memcpy(CoW_bounce_buffer, (void*)(virtual_address & ~0xFFF), 0x1000);
                uint16_t flags = (*page_data & 0xFFF) | 0b10;
                uint8_t xd = *page_data >> 63;
                *page_data = (
                    phys | 
                    (((uint64_t)xd) << 63) | 
                    flags
                );
                asm volatile("invlpg (%0)" : : "r" (virtual_address & ~0xFFF) : "memory");
                memcpy((void*)(virtual_address & ~0xFFF), CoW_bounce_buffer, 0x1000);
            }
        }
        mem_set_cr3(cur_task->Base_PML4, false);
        PageDetails newUser;
        newUser.flags.flags = USER_FLAGS;
        newUser.flags.Execute_Disable = false;
        newUser.physical_address = newPhysical;
        newUser.virtual_address = virtual_address;

        alloc_page(&newUser);
    }else{
        #ifdef DEBUG
            printf("Kernel\n");
        #endif
        PageDetails newKernel;
        newKernel.flags.flags = KERNEL_FLAGS;
        newKernel.flags.Execute_Disable = false;
        newKernel.physical_address = newPhysical;
        newKernel.virtual_address = virtual_address;

        alloc_page(&newKernel);
    }
    SetTextColor(WHITE, BLACK);
}

void GeneralProtectionFault(InterruptRegistersError* regs){
    int cur = TASKMGR_get_current();
    printf("#GF RIP=%x (Task=%i)\n", regs->rip, cur);

    KillTask(cur);
    InterruptRegisters regs_i = ErrToInt(*regs);
    ForceSwitch(&regs_i);
    return;

    asm(
        "cli\n"
        "1:\n\t"
        "hlt\n"
        "jmp 1b\n"
        :::
    );

    return;
}

void InvalidOpcode(InterruptRegistersError* regs){
    int cur = TASKMGR_get_current();
    printf("Invalid opcode at RIP %x (Task %i)\n", regs->rip, cur);

    // kill the task that the opcode originated from
    KillTask(cur);
    InterruptRegisters regs_i = ErrToInt(*regs);
    ForceSwitch(&regs_i);
    return;

    asm volatile(
        "cli\n"
        "1:\n\t"
        "hlt\n"
        "jmp 1b\n"
        :::
    );
    
    return;
}