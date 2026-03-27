#include "ErrorHandling/KernelPanic.h"
#include "LowLevel/Memory.h"
#include "Userland/Tasks.h"

void KernelPanic(InterruptRegistersError* regs){
    uint64_t CR3;
    asm volatile(
        "movq %%cr3, %%rax\n"
        "movq %%rax, %0\n"
        : "=r"(CR3) : :
    );

    printf("HAPPENING ALERT FROM PID %i!!!!\n\t", TASKMGR_get_current());
    printf("General Registers:\n\t\t");
    printf("RAX: %x ", regs->rax);
    printf("RBX: %x ", regs->rbx);
    printf("RCX: %x ", regs->rcx);
    printf("RDX: %x\n\t\t", regs->rdx);
    printf("RDI: %x ", regs->rdi);
    printf("RSI: %x\n\t\t", regs->rsi);
    printf("r10: %x\n\t\t", regs->r10);
    printf("r11: %x\n\t\t", regs->r11);
    printf("r12: %x\n\t\t", regs->r12);
    printf("r13: %x\n\t\t", regs->r13);
    printf("r14: %x\n\t\t", regs->r14);
    printf("r15: %x\n\t", regs->r15);
    printf("Special Registers:\n\t\t");
    printf("RIP: %x\n\t\t", regs->rip);
    printf("RSP: %x\n\t\t", regs->rsp);
    printf("RBP: %x\n\t\t", regs->rbp);
    printf("CR2: %x\n\t\t", regs->cr2);
    printf("CR3: %x\n\t\t", CR3);
    printf("CS: %x\n\t\t", regs->cs);
    printf("SS: %x\n\t\t", regs->ss);
    printf("RFLAGS: %x\n\t\t", regs->rflags);
    printf("ERROR CODE: %x\n", regs->error_code);
    if(regs->rip >= VIRTUAL_MEMORY_BARRIER){
        printf("This panic seems to have originated from kernel space, please report this on the github issues page.\n");
    }else{
        printf("This panic seems to have originated from user space.");
    }
    
    printf("You can now turn off your computer.");

    asm(
        "cli\n"
        "1:\n\t"
        "hlt\n"
        "jmp 1b\n"
        :
    );
}