#include "ErrorHandling/Exceptions.h"

volatile uint8_t CoW_bounce_buffer[0x1000];

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

int pf_count = 0;

void HandlePageFault(InterruptRegisters* regs){
    uint64_t virtual_address = regs->cr2;

    uint64_t CR3;
    asm volatile(
        "movq %%cr3, %%rax\n"
        "movq %%rax, %0\n"
        : "=r" (CR3) : :
    );

    int cur_pid = TASKMGR_get_current();

    task_switch_frame(&TaskManager[cur_pid].SavedRegisters, regs);

    uint64_t saved_virt = virtual_address;

    virtual_address = virtual_address & ~0xFFF;

    volatile Task* cur_task = (volatile Task*)&TaskManager[cur_pid];

    uint64_t stack_low = (cur_task->MemoryData.StackBaseVirtualAddress - ((cur_task->MemoryData.StackPageCount-1) * 0x1000));

    if(
        !mem_access_ok(saved_virt, cur_pid) &&
        virtual_address < (stack_low - 0x4000)
    ){
        virtualprint(KERNEL_T, "[exc.pf] kill task\n");
        DumpTaskState(cur_pid);
        KillTask(cur_pid);
        SignalOwner(cur_task, CHILD_WAIT_EXIT, -1);
        free_task_memory(cur_pid);
        ForceSwitch(regs);

        return;
    }

    if(virtual_address < VIRTUAL_MEMORY_BARRIER){
        mem_set_cr3(cur_task->Base_PML4, false);
        PageEntries entries = ExtractPageEntries(virtual_address);
        volatile uint64_t* page_data = (volatile uint64_t*)CalculatePagePhysicalEntryAddress(&entries);

        // if CoW bit of the page entry is set, and the P and W bits of the error is set, then it should copy.
        if(*page_data & 1<<9){
            // find a free physical, if it doesn't exist kill the task
            uint16_t flags = ((*page_data & 0xFFF) | 0b11) & (~(1 << 9)); // enable writes and disable CoW
            uint8_t xd = (*page_data) >> 63;
            if(phys_frames[(*page_data & 0x000FFFFFFFFFF000ULL) / 0x1000].refcount <= 1){
                *page_data = (
                    (*page_data & 0x000FFFFFFFFFF000ULL) | 
                    (((uint64_t)xd) << 63) | 
                    flags
                );
                asm volatile("invlpg (%0)" : : "r" (virtual_address) : "memory");
            }else{
                phys_frames[(*page_data & 0x000FFFFFFFFFF000ULL) / 0x1000].refcount--;
                *page_data = (
                    (*page_data & 0x000FFFFFFFFFF000ULL) | 
                    (((uint64_t)xd) << 63) | 
                    flags
                );
                uint64_t newPhysical = FindNextFreePhysical();
                memcpy(CoW_bounce_buffer, (void*)virtual_address, 0x1000);
                *page_data = (
                    newPhysical | 
                    (((uint64_t)xd) << 63) | 
                    flags
                );
                asm volatile("invlpg (%0)" : : "r" (virtual_address) : "memory");
                mem_SetBit(newPhysical);
                barrier();
                memcpy((void*)virtual_address, CoW_bounce_buffer, 0x1000);
            }
        }else{
            uint64_t newPhysical = FindNextFreePhysical();
            PageDetails newUser;
            newUser.flags.flags = USER_FLAGS;
            newUser.flags.Execute_Disable = false;
            newUser.physical_address = newPhysical;
            newUser.virtual_address = virtual_address;

            if(
                virtual_address >= (stack_low - 0x2000) &&
                virtual_address <= cur_task->MemoryData.StackBaseVirtualAddress
            ){
                uint64_t dist = (stack_low - virtual_address) / 0x1000;
                
                newUser.flags.Execute_Disable = true;
                for(int i = 0; i < dist; i++){
                    alloc_page(&newUser);

                    newUser.virtual_address+=0x1000;
                    newUser.physical_address = FindNextFreePhysical();

                    cur_task->MemoryData.StackPageCount++;   
                }
            }else{
                cur_task->MemoryData.PageCount+=1;
                alloc_page(&newUser);
            }
        }
    }else{
        uint64_t newPhysical = FindNextFreePhysical();
        PageDetails newKernel;
        newKernel.flags.flags = KERNEL_FLAGS;
        newKernel.flags.Execute_Disable = false;
        newKernel.physical_address = newPhysical;
        newKernel.virtual_address = virtual_address;

        alloc_page(&newKernel);
    }
}

void GeneralProtectionFault(InterruptRegistersError* regs){
    int cur = TASKMGR_get_current();

    if(cur == 0){
        virtualprint(KERNEL_T, "[exc.gf] kernel fail\n");
        KernelPanic(regs);
    }

    virtualprint(KERNEL_T, "[exc.gf] fatal. kill task\n");
    KillTask(cur);
    InterruptRegisters regs_i = ErrToInt(*regs);
    ForceSwitch(&regs_i);
}

void InvalidOpcode(InterruptRegistersError* regs){
    int cur = TASKMGR_get_current();

    if(cur == 0){
        virtualprint(KERNEL_T, "[exc.op] fatal kernel\n");
        KernelPanic(regs);
    }

    // kill the task that the opcode originated from
    virtualprint(KERNEL_T, "[exc.op] fatal. kill task\n");
    KillTask(cur);
    InterruptRegisters regs_i = ErrToInt(*regs);
    ForceSwitch(&regs_i);
}