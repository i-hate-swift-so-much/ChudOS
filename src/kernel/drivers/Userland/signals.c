#include "Userland/signals.h"

/*
    Push order of signal stack
    rsp
    rip
    rax
    rbx
    rcx
    rdx
    rbp
    rsi
    rdi
    r8
    r9
    r10
    r11
    r12
    r13
    r14
    r15
*/
void EstablishSignalStack(int pid){
    volatile Task* task = (volatile Task*) &TaskManager[pid];
    if(!task->Exists){ return; }
    int curPid = TASKMGR_get_current();
    bool isCurrent = task->ProcessID == curPid;
    volatile uint64_t last = PML4_Physical;
    if(!isCurrent){
        mem_set_cr3(task->Base_PML4, true);
    }

    task->ProcessState = CREATION_PROCESS_STATE;
    
    TaskStack_Push(pid, task->SavedRegisters.rsp, false);
    TaskStack_Push(pid, task->SavedRegisters.rip, false);
    TaskStack_Push(pid, task->SavedRegisters.rax, false);
    TaskStack_Push(pid, task->SavedRegisters.rbx, false);
    TaskStack_Push(pid, task->SavedRegisters.rcx, false);
    TaskStack_Push(pid, task->SavedRegisters.rdx, false);
    TaskStack_Push(pid, task->SavedRegisters.rbp, false);
    TaskStack_Push(pid, task->SavedRegisters.rdi, false);
    TaskStack_Push(pid, task->SavedRegisters.rsi, false);
    TaskStack_Push(pid, task->SavedRegisters.r9, false);
    TaskStack_Push(pid, task->SavedRegisters.r10, false);
    TaskStack_Push(pid, task->SavedRegisters.r11, false);
    TaskStack_Push(pid, task->SavedRegisters.r12, false);
    TaskStack_Push(pid, task->SavedRegisters.r13, false);
    TaskStack_Push(pid, task->SavedRegisters.r14, false);
    TaskStack_Push(pid, task->SavedRegisters.r15, false);

    /*
    TaskStack_Push(pid, 0x0DEADBEEF, false);
    TaskStack_Push(pid, 0x1DEADBEEF, false);
    TaskStack_Push(pid, 0x2DEADBEEF, false);
    TaskStack_Push(pid, 0x3DEADBEEF, false);
    TaskStack_Push(pid, 0x4DEADBEEF, false);
    TaskStack_Push(pid, 0x5DEADBEEF, false);
    TaskStack_Push(pid, 0x6DEADBEEF, false);
    TaskStack_Push(pid, 0x7DEADBEEF, false);
    TaskStack_Push(pid, 0x8DEADBEEF, false);
    TaskStack_Push(pid, 0x9DEADBEEF, false);
    TaskStack_Push(pid, 0xADEADBEEF, false);
    TaskStack_Push(pid, 0xBDEADBEEF, false);
    TaskStack_Push(pid, 0xCDEADBEEF, false);
    TaskStack_Push(pid, 0xDDEADBEEF, false);
    TaskStack_Push(pid, 0xEDEADBEEF, false);
    TaskStack_Push(pid, 0xFDEADBEEF, false);
    */

    // fuck i figured it out. use a non volatile register to save the start of the struct, then use that to return.
    task->SavedRegisters.r12 = task->SavedRegisters.rsp;

    if(!isCurrent){
        mem_set_cr3(last, true);
    }

    task->ProcessState = READY_PROCESS_STATE;
}

void RestoreTaskState(int pid){
    
}

void SignalReturn(int pid){
    volatile Task* task = (volatile Task*) &TaskManager[pid];
    if(!task->Exists){ return; }
    bool isCurrent = pid == TASKMGR_get_current();
    volatile uint64_t last = PML4_Physical;
    if(!isCurrent){
        mem_set_cr3(task->Base_PML4, true);
    }

    task->ProcessState = CREATION_PROCESS_STATE;

    task->SavedRegisters.rsp = task->SavedRegisters.r12;

    task->SavedRegisters.r15 = TaskStack_Pop(pid, false);
    task->SavedRegisters.r14 = TaskStack_Pop(pid, false);
    task->SavedRegisters.r13 = TaskStack_Pop(pid, false);
    task->SavedRegisters.r12 = TaskStack_Pop(pid, false);
    task->SavedRegisters.r11 = TaskStack_Pop(pid, false);
    task->SavedRegisters.r10 = TaskStack_Pop(pid, false);
    task->SavedRegisters.r9 = TaskStack_Pop(pid, false);
    task->SavedRegisters.rsi = TaskStack_Pop(pid, false);
    task->SavedRegisters.rdi = TaskStack_Pop(pid, false);
    task->SavedRegisters.rbp = TaskStack_Pop(pid, false);
    task->SavedRegisters.rdx = TaskStack_Pop(pid, false);
    task->SavedRegisters.rcx = TaskStack_Pop(pid, false);
    task->SavedRegisters.rbx = TaskStack_Pop(pid, false);
    task->SavedRegisters.rax = TaskStack_Pop(pid, false);
    task->SavedRegisters.rip = TaskStack_Pop(pid, false);
    task->SavedRegisters.rsp = TaskStack_Pop(pid, false);

    if(!isCurrent){
        mem_set_cr3(last, true);
    }
    
    task->ProcessState = READY_PROCESS_STATE;
}

void SendSignal(int pid, enum SignalIdentifiers ident, uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3){
    volatile Task* task = (volatile Task*) &TaskManager[pid];
    if(task->Exists == false || task->Signals[ident].registered == false){ return; }

    task->ProcessState = CREATION_PROCESS_STATE;

    if(!mem_access_ok(task->Signals[ident].entry, pid)){ printf("Couldn't find signal %x registered to task %i\n", ident, pid); return; }
    
    EstablishSignalStack(pid);
    task->SavedRegisters.rdi = arg0;
    task->SavedRegisters.rsi = arg1;
    task->SavedRegisters.rdx = arg2;
    task->SavedRegisters.rcx = arg3;

    task->SavedRegisters.rip = task->Signals[ident].entry;

    task->ProcessState = READY_PROCESS_STATE;
}

void RegisterSignal(int pid, enum SignalIdentifiers ident, uint64_t entry){
    volatile Task* task = (volatile Task*) &TaskManager[pid];
    if(task->Exists == false || !mem_access_ok(entry, pid)){ return; }
    task->Signals[ident].entry = entry;
    task->Signals[ident].registered = true;
}