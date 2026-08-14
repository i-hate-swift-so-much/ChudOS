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

    pic_mask(0x00);

    switch (rax_value){
        case 0x0:{
            // EXIT
            // RBX = return code
            if(cur_task->Owner_PID != 0){
                SendSignal(cur_task->Owner_PID, SIGCHD, 0x00, rbx_value, 0x00, 0x00);
            }
            KillTask(cur);
            //SignalOwner(cur_task, CHILD_WAIT_EXIT, rbx_value); // legacy
            free_task_memory(cur);
            ForceSwitch(regs);
            pic_unmask(0x00);
            return;
            break;}
        case 0x01:{
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
                regs->rax = -1;
                break;
            }
            len++;
            memcpy(open_bounce_buffer, (void*)rbx_value, len);
            if(len > 255){ len = 255; }
            uint64_t index = GemFS_Directory_to_Index(0, 1, open_bounce_buffer);
            if(index != -1){
                descriptor->used = true;
                descriptor->flags = 0b0;
                descriptor->gemfs_index =  index;
                regs->rax = (uint64_t)fd;
            }else{
                regs->rax = -1;
            }
            break;}
        case 0x02:{
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
            break;}
        case 0x03:{
            // READ
            // rdx = descriptor
            // rcx - byte count
            // rbx = buffer address
            switch(rdx_value){
                case STD_IN_FD:{
                    // in the case of reading from the standard input, the task will wait until an enter, and the input will be sent directly to the task.
                    // this is used primarily by SHELL.
                    TaskManager[cur].ProcessState = WAITING_PROCESS_STATE;
                    TaskManager[cur].WaitingReason = WAITING_REASON_INPUT;

                    ForceSwitch(regs);
                    pic_unmask(0x00);
                    return;
                    break;
                }
                default:{
                    descriptor = &TaskManager[cur].Descriptors[rdx_value];
                    if(!descriptor->used){break;}
                    GemFS_ReadFile(0, 1, descriptor->gemfs_index, (void*)bounce_buffer, (rcx_value/512)+1);
                    memcpy((void*)rbx_value, bounce_buffer, rcx_value);
                    break;
                }
            }
            
            break;}
        case 0x04:{
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
            child_task->Owner_PID = cur;

            cur_task->SavedRegisters.rax = child_pid;
            regs->rax = child_pid;

            //printf("FORK NEW TASK %i\nRIP %x\nPML4 %x\nRAX %x\n", child_pid, child_task->SavedRegisters.rip, child_task->Base_PML4, child_task->SavedRegisters.rax);

            uint64_t cur_page = cur_task->MemoryData.BaseVirtualAddress;

            // setup the child pages, mark the parent and child pages to be read only, CoW available, and update the ref count
            for(int i = 0; i < cur_task->MemoryData.PageCount; i++){
                mem_set_cr3(cur_task->Base_PML4, false);
                TASKMGR_set_current(cur);
                
                PageEntries entries = ExtractPageEntries(cur_page);
                uint64_t* page_data = CalculatePagePhysicalEntryAddress(&entries);

                *page_data |= (1 << 9); // CoW
                *page_data &= ~2; // read only

                PageDetails page;
                page.virtual_address = cur_page;
                page.physical_address = *page_data & 0x000FFFFFFFFFF000ULL;
                page.flags.flags = *page_data & 0xFFFULL;
                page.flags.Execute_Disable = false;

                mem_set_cr3(child_task->Base_PML4, false);
                TASKMGR_set_current(child_pid);

                alloc_page(&page);
                cur_page+=0x1000;
            }

            child_task->MemoryData.StackBaseVirtualAddress = cur_task->MemoryData.StackBaseVirtualAddress;
            child_task->MemoryData.StackPageCount = cur_task->MemoryData.StackPageCount;

            cur_page = cur_task->MemoryData.StackBaseVirtualAddress;
            //printf("alloc %x stack pages at %x\n", cur_task->MemoryData.StackPageCount, cur_task->MemoryData.StackBaseVirtualAddress);
            // do the same for the stack
            for(int i = 0; i < cur_task->MemoryData.StackPageCount; i++){
                mem_set_cr3(cur_task->Base_PML4, false);
                TASKMGR_set_current(cur);
                
                volatile PageEntries entries = (volatile PageEntries)ExtractPageEntries(cur_page);
                volatile uint64_t* page_data = (volatile uint64_t*)CalculatePagePhysicalEntryAddress(&entries);

                *page_data |= (1 << 9); // CoW
                *page_data &= ~2; // read only

                PageDetails page;
                page.virtual_address = cur_page;
                page.physical_address = *page_data & 0x000FFFFFFFFFF000ULL;
                page.flags.flags = *page_data & 0xFFFULL;
                page.flags.Execute_Disable = false;

                mem_set_cr3(child_task->Base_PML4, false);
                TASKMGR_set_current(child_pid);

                alloc_page(&page);
                cur_page-=0x1000;
            }

            mem_set_cr3(cur_task->Base_PML4, false);
            TASKMGR_set_current(cur);
            
            //printf("alloc %x kernel stack pages at %x\n", cur_task->MemoryData.KernelStackPageCount, cur_task->MemoryData.KernelStackBaseVirtualAddress);

            //create a new kernel stack for the child.
            child_task->MemoryData.KernelStackPageCount = 5;
            malloc(KernelTask);
            malloc(KernelTask);
            malloc(KernelTask);
            malloc(KernelTask);
            uint64_t kernel_stack = (uint64_t)malloc(KernelTask);
            child_task->MemoryData.KernelStackBaseVirtualAddress = kernel_stack;

            child_task->MemoryData.KernelStackPageCount = 5;
            child_task->MemoryData.KernelStackBaseVirtualAddress = kernel_stack;

            //printf("Child PML4: %x\nParent PML4: %x\n", child_task->Base_PML4, cur_task->Base_PML4);

            mem_set_cr3(cur_task->Base_PML4, true);
            TASKMGR_set_current(cur);

            child_task->Owner_PID = cur;
            child_task->ProcessState = READY_PROCESS_STATE;
            //DumpTaskState(child_pid);
            break;}
        case 0x05:{
            // EXECVE
            // RAX = 0x5
            // RBX = argv
            // RDX = FILE DESCRIPTOR
            int owner = cur_task->Owner_PID;
            descriptor = &cur_task->Descriptors[rdx_value];
            if(!descriptor->used){ break; }
            // verify that the file is executable
            struct GemFS_Entry target_entry = GemFS_ReadEntry(F0, 1, descriptor->gemfs_index);
            if((target_entry.Flags & 0b1000) != 0b1000){ break; }

            // if the file is executable, then continue.
            uint64_t old_pml4 = cur_task->Base_PML4;
            uint64_t new_pml4 = Create_User_Memory();
            pic_mask(0x00);

            char** argv = (char**)rbx_value;
            uintptr_t* list = (uintptr_t*)argv;
            int len = 0;
            while(list[len++] != 0){}

            // copy the given argv from user to kernel space
            char** kargv;
            char points[len*sizeof(char*)];
            copy_from_user(points, (void*)rbx_value, len*sizeof(char*));

            cur_task->ProcessState = CREATION_PROCESS_STATE;
            
            LoadElfStrict_GemFS(F0, 1, 3, descriptor->gemfs_index, TASKMGR_get_current(), new_pml4, (char**)rbx_value, 0);

            free_task_memory_by_pml4(old_pml4);

            memcpy(regs, &cur_task->SavedRegisters, sizeof(InterruptRegisters));

            cur_task->Owner_PID = owner;
            cur_task->ProcessState = READY_PROCESS_STATE;

            ForceSwitch(regs);

            pic_unmask(0x00);

            return;
        }
        case 0x06:{
            //printf("WAIT FROM %x FOR %x REASON %x\n", cur, rbx_value, rdx_value);
            // WAIT
            // RBX = PID
            // RDX = enum ChildWaitingReasons
            volatile Task* target_task = (volatile Task*)&TaskManager[rbx_value];
            //printf("no gf");
            if(target_task->Owner_PID != cur){ break; }
            cur_task->ProcessState = WAITING_PROCESS_STATE;
            cur_task->WaitingReason = WAITING_REASON_CHILD;
            cur_task->ChildWaitingReason = rdx_value;
            ForceSwitch(regs);
            break;
        }
        case 0x07:{
            // CLOSE
            // RDX = file decsriptor
            cur_task->Descriptors[rdx_value].used = false;

            break;
        }
        case 0x0A:{
            // SETWFD
            // RAX = 0x0A
            // RDX = DESCRIPTOR
            cur_task->Working_FD = rdx_value;
            break;}
        case 0x0B:{
            // GETWFD
            // RAX = 0x0B
            // RETURN WITH WFD
            regs->rax = cur_task->Working_FD;
            break;}
        case 0x0C:{
            // GETDENTS
            // RAX = 0x0C
            // RBX = struct dent*
            // RCX = Buffer length
            // RDX = File Descriptor
            if(cur_task->Descriptors[rdx_value].used == false){
                break;
            }
            struct dent cur_dent;
            struct GemFS_Entry cur_entry = GemFS_ReadEntry(F0, 1, rdx_value);
            struct dent* user_buffer = (struct dent*)rbx_value;
            int i = 0;
            while(i < rcx_value && cur_entry.Sibling_Index != 0){
                cur_entry = GemFS_ReadEntry(F0, 1, cur_entry.Sibling_Index);
                memcpy(&cur_dent.Name, &cur_entry.Name, 128);
                cur_dent.NameLen = 0;
                while(cur_entry.Name[cur_dent.NameLen++] != 0){}
                cur_dent.Flags = cur_entry.Flags;
                memcpy(&cur_dent, &user_buffer[i], sizeof(struct dent));
                i++;
            }
            regs->rax = i;
            break;
        }
        case 0x0D:{
            // SYSINFO
            // RAX = 0x0D
            // RBX = Pointer to struct sysinfo
            struct sysinfo* user_info = (struct sysinfo*)rbx_value;
            struct sysinfo main_info;

            main_info.sys_mem_total = total_mem;
            main_info.sys_mem_avl = available_mem - (PhysicalPagesUsed*0x1000);
            main_info.sys_mem_used = PhysicalPagesUsed*0x1000;
            main_info.your_mem_size = cur_task->MemoryData.PageCount;
            main_info.your_mem_size += cur_task->MemoryData.StackPageCount;
            main_info.your_mem_size += cur_task->MemoryData.KernelStackPageCount;
            main_info.your_mem_size *= 0x1000;

            memcpy(user_info, &main_info, sizeof(struct sysinfo));

            break;
        }
        case 0x0E:{
            // sigreg
            // RAX = 0x0E
            // RBX = IDENT
            // RCX = ENTRY

            RegisterSignal(cur, rbx_value, rcx_value);

            //printf("Task %i registered signal %x at %x\n", cur, rbx_value, rcx_value);

            break;
        }
        case 0x0F:{
            // sigret
            // RAX = 0x0F

            SignalReturn(cur);

            //printf("Task %i returned from signal\n", cur);
            task_switch_frame(regs, &TaskManager[cur].SavedRegisters);

            break;
        }
    }
    task_switch_frame(&TaskManager[cur].SavedRegisters, regs);
    pic_unmask(0x00);
}