#include "ErrorHandling/Exceptions.h"

void HandlePageFault(InterruptRegistersError* regs){
    PageEntries newPhysical = FindNextFreePhysical();
    uint64_t virtual_address = regs->cr2;

    SetTextColor(LRED, BLACK);

    uint64_t CR3;
    asm volatile(
        "movq %%cr3, %%rax\n"
        "movq %%rax, %0\n"
        : "=r" (CR3) : :
    );

    #ifdef DEBUG
        printf("(!) Page fault\n\tCR2: %x\n\tERR: %b\n\tRIP: %x\n\tCR3: %x\n\tFrom: ", virtual_address, regs->error_code, regs->rip, CR3);
   
        if(regs->rip < VIRTUAL_MEMORY_BARRIER){
            printf("User | For: ");
        }else{
            printf("Kernel | For: ");
        }
    #endif

    virtual_address = virtual_address & ~0xFFF;

    if(virtual_address < VIRTUAL_MEMORY_BARRIER){
        #ifdef DEBUG
            printf("User\n");
        #endif
        PageDetails newUser;
        newUser.flags.flags = USER_FLAGS;
        newUser.flags.Execute_Disable = false;
        newUser.physical_address = CalculatePageAddress(&newPhysical);
        newUser.virtual_address = virtual_address;

        alloc_page(&newUser);
    }else{
        #ifdef DEBUG
            printf("Kernel\n");
        #endif
        PageDetails newKernel;
        newKernel.flags.flags = KERNEL_FLAGS;
        newKernel.flags.Execute_Disable = false;
        newKernel.physical_address = CalculatePageAddress(&newPhysical);
        newKernel.virtual_address = virtual_address;

        alloc_page(&newKernel);
    }
    SetTextColor(WHITE, BLACK);
}

void GeneralProtectionFault(InterruptRegistersError* regs){
    print("General Protection Fault\n", 0);

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
    printf("Invalid opcode at RIP %x\n", regs->rip);

    asm volatile(
        "cli\n"
        "1:\n\t"
        "hlt\n"
        "jmp 1b\n"
        :::
    );
    
    return;
}