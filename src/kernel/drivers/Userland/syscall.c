#include "Userland/syscall.h"

int curRow = 0;
int curCol = 0;

#define STD_IN_FD 0
#define STD_OUT_FD 1

/*
int 0x80 (syscall) can only be triggered from user space because it requires the CPU to automatically push
registers RSP and SS, which are only passed in user mode. If called from kernel mode, it will result in a
garbage struct being pushed, it will push the RegistersKernelCall struct which is defined in "IDT.h" alongside
the RegistersUsersCall struct, and does not feature RSP and SS, which will corrupt the stack.
*/

int lastPrintX = 0;
int lastPrintY = 0;

uint8_t bounce_buffer[4096];

void handle_syscall(InterruptRegisters* regs){
    uint64_t rax_value = regs->rax;
    uint64_t rbx_value = regs->rbx;
    uint64_t rcx_value = regs->rcx;
    uint64_t rdx_value = regs->rdx;

    int cur = TASKMGR_get_current();

    task_switch_frame(&TaskManager[cur].SavedRegisters, regs);

    //printf("SYSCALL %i FROM %i\n", regs->rax, cur);

    volatile Task* cur_task = (volatile Task*)&TaskManager[cur];

    struct FileDescriptor* descriptor;

    switch (rax_value){
        case 0:
            KillTask(cur);
            ForceSwitch(regs);
            return;
        case 1:
            // OPEN
            // rbx = directory buffer address
            // sets rax to the new descriptor
            int fd = FindFreeFileDescriptor(cur);
            descriptor = &TaskManager[cur].Descriptors[fd];
            char open_bounce_buffer[256];
            memset(open_bounce_buffer, 0, 256);
            memset(descriptor, 0, sizeof(struct FileDescriptor));
            int len = calculate_string_length((void*)rbx_value);
            len++;
            memcpy(open_bounce_buffer, (void*)rbx_value, len);
            if(len > 255){ len = 255; }
            memcpy(&descriptor->directory, open_bounce_buffer, len);
            descriptor->directory[len] = '\0';
            descriptor->used = true;
            descriptor->flags = 0b0;
            descriptor->gemfs_index =  GemFS_Directory_to_Index(0, 1, descriptor->directory);
            regs->rax = (uint64_t)fd;
            break;
        case 2:
            // WRITE
            // rdx = descriptor
            // rcx = byte count
            // rbx = buffer address
            //printf("WRITE FROM %x TO FD %i\n", rbx_value, rdx_value);
            switch (rdx_value){
                case STD_OUT_FD:
                    memcpy(bounce_buffer, (void*)rbx_value, rcx_value);
                    print(bounce_buffer, rcx_value);
                    break;
                default:
                    descriptor = &TaskManager[cur].Descriptors[rdx_value];
                    memcpy(bounce_buffer, (void*)rbx_value, rcx_value);
    
                    GemFS_WriteFile(bounce_buffer, (rcx_value/512)+1, F0, 1, descriptor->gemfs_index);
                    break;
            }
            break;
        case 3:
            // READ
            // rdx = descriptor
            // rcx - byte count
            // rbx = buffer address
            descriptor = &TaskManager[cur].Descriptors[rdx_value];
            GemFS_ReadFile(0, 1, descriptor->gemfs_index, (void*)bounce_buffer, (rcx_value/512)+1);
            memcpy((void*)rbx_value, bounce_buffer, rcx_value);
            break;
        case 4:
            // FORK
            // for parent, return child PID. for child, return 0.
            // uses Copy on write for copying data.

            int child_pid = RegisterTask(
                cur_task->MemoryData.BaseVirtualAddress, 
                regs->rip-cur_task->MemoryData.BaseVirtualAddress,
                cur_task->MemoryData.PageCount,
                cur_task->MaxTicks,
                regs->rbp,
                cur_task->Base_PML4
            );
            volatile Task* child_task = (volatile Task*)&TaskManager[child_pid];
            memcpy((void*)child_task, (void*)cur_task, sizeof(struct Task_S));
            task_switch_frame(&child_task->SavedRegisters, regs);
            child_task->SavedRegisters.rax = 0;
            child_task->ProcessID = child_pid;
            child_task->ProcessState = CREATION_PROCESS_STATE;
            child_task->Exists = true;
            child_task->Base_PML4 = Create_User_Memory();

            cur_task->SavedRegisters.rax = child_pid;
            regs->rax = child_pid;

            //printf("FORK NEW TASK %i\nRIP %x\nPML4 %x\nRAX %x\n", child_pid, child_task->SavedRegisters.rip, child_task->Base_PML4, child_task->SavedRegisters.rax);

            uint64_t cur_page = cur_task->MemoryData.BaseVirtualAddress;

            // setup the child pages, mark the parent and child pages to be read only, CoW available, and update the ref count
            for(int i = 0; i < cur_task->MemoryData.PageCount; i++){
                mem_set_cr3(cur_task->Base_PML4, true);
                TASKMGR_set_current(cur);
                
                PageEntries entries = ExtractPageEntries(cur_page);

                uint64_t* page_data = CalculatePagePhysicalEntryAddress(&entries);
                *page_data |= (1 << 9); // CoW
                *page_data &= ~2; // read only
                //printf("%b", *page_data);

                mem_set_cr3(child_task->Base_PML4, true);
                TASKMGR_set_current(child_pid);

                PageDetails page;
                page.virtual_address = cur_page;
                page.physical_address = *page_data & 0x000FFFFFFFFFF000ULL;
                page.flags.flags = *page_data & 0xFFFULL;
                page.flags.Execute_Disable = false;

                //printf("NEW PAGE: V: %x P: %x F: %b\n", page.virtual_address, page.physical_address, page.flags.flags);

                alloc_page(&page);
                cur_page+=0x1000;
            }

            mem_set_cr3(cur_task->Base_PML4, true);
            TASKMGR_set_current(cur);

            child_task->ProcessState = READY_PROCESS_STATE;
    }

    task_switch_frame(&TaskManager[cur].SavedRegisters, regs);
}