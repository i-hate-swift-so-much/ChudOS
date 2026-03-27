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

uint8_t bounce_buffer[0x1000];
char open_bounce_buffer[256];

uint64_t last_stdout = 0;

uint64_t copy_from_user(void* dest, void* src, size_t bytes){
    uint64_t phys = phys_addr(src);
    void* new_src = (void*)(phys+VIRTUAL_MEMORY_BARRIER);
    memcpy(dest, new_src, bytes);
}

void handle_syscall(InterruptRegisters* regs){
    uint64_t rax_value = regs->rax;
    uint64_t rbx_value = regs->rbx;
    uint64_t rcx_value = regs->rcx;
    uint64_t rdx_value = regs->rdx;

    int cur = TASKMGR_get_current();

    task_switch_frame(&TaskManager[cur].SavedRegisters, regs);

    //printf("SYSCALL %i FROM %i RBX=%x RCX=%x RDX=%x\n", regs->rax, cur, regs->rbx, regs->rcx, regs->rdx);

    volatile Task* cur_task = (volatile Task*)&TaskManager[cur];
    mem_set_cr3(cur_task->Base_PML4, true);

    struct FileDescriptor* descriptor;

    switch (rax_value){
        case 0:
            KillTask(cur);
            ForceSwitch(regs);
            break;
        case 1:
            // OPEN
            // rbx = directory buffer address
            // sets rax to the new descriptor
            uint8_t* check = (uint8_t*)rbx_value;
            memset(open_bounce_buffer, 0, 256);
            int fd = FindFreeFileDescriptor(cur);
            descriptor = &TaskManager[cur].Descriptors[fd];
            memset(descriptor, 0, sizeof(struct FileDescriptor));
            int len = calculate_string_length((void*)rbx_value);
            if(len == 0){
                break;
            }
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
                    memset(bounce_buffer, 0, 0x1000);
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
                regs->rip,
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

                //printf("NEW PAGE: V: %x P: %x F: %b (D)\n", page.virtual_address, page.physical_address, page.flags.flags);

                alloc_page(&page);
                cur_page+=0x1000;
            }

            child_task->MemoryData.StackBaseVirtualAddress = cur_task->MemoryData.StackBaseVirtualAddress;
            child_task->MemoryData.StackPageCount = cur_task->MemoryData.StackPageCount;

            cur_page = cur_task->MemoryData.StackBaseVirtualAddress;
            //printf("alloc %x stack pages at %x\n", cur_task->MemoryData.StackPageCount, cur_task->MemoryData.StackBaseVirtualAddress);
            // do the same for the stack
            for(int i = 0; i < cur_task->MemoryData.StackPageCount; i++){
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

                //printf("NEW PAGE: V: %x P: %x F: %b (S)\n", page.virtual_address, page.physical_address, page.flags.flags);

                alloc_page(&page);
                cur_page+=0x1000;
            }

            mem_set_cr3(cur_task->Base_PML4, true);
            TASKMGR_set_current(cur);

            child_task->ProcessState = READY_PROCESS_STATE;
            break;
        case 5:
            // EXECVE
            // RAX = 0x5
            // RDX = FILE DESCRIPTOR
            descriptor = &cur_task->Descriptors[rdx_value];
            if(!descriptor->used){ break; }
            // verify that the file is executable
            struct GemFS_Entry target_entry = GemFS_ReadEntry(F0, 1, descriptor->gemfs_index);
            printf("Index %x\n", descriptor->gemfs_index);
            if((target_entry.Flags & 0b1000) != 0b1000){ break; }

            // if the file is executable, then continue.
            uint64_t new_pml4 = Create_User_Memory();
            mem_set_cr3(new_pml4, true);
            pic_mask(0x00);

            //printf("new pml4 %x\n", new_pml4);

            PML4_Physical = cur_task->Base_PML4;

            free_task_memory(cur);

            printf("task\n");

            PML4_Physical = new_pml4;

            //LoadElfStrict_GemFS(F0, 1, 3, descriptor->gemfs_index, TASKMGR_get_current(), new_pml4);
            LoadElfStrict(0, 1600, 53, TASKMGR_get_current(), new_pml4);

            memcpy(regs, &cur_task->SavedRegisters, sizeof(InterruptRegisters));

            pic_unmask(0x00);

            break;
    }

    task_switch_frame(&TaskManager[cur].SavedRegisters, regs);
}