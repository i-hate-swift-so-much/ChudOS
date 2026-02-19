#include "ErrorHandling/Exceptions.h"

void HandlePageFault(InterruptRegistersError* regs){
    printf("Page fault\n");

    PageEntries newPhysical = FindNextFreePhysical();
    uint64_t virtual_address = regs->cr2;

    void* confirmation;

    if(virtual_address > VIRTUAL_MEMORY_BARRIER){
        printf("User\n");
        
        PageDetails newUser;
        newUser.flags.flags = USER_FLAGS;
        newUser.flags.Execute_Disable = false;
        newUser.physical_address = CalculatePageAddress(&newPhysical);
        newUser.virtual_address = virtual_address;

        alloc_page(&newUser);
    }else{
        printf("Kernel\n");
        PageDetails newKernel;
        newKernel.flags.flags = KERNEL_FLAGS;
        newKernel.flags.Execute_Disable = false;
        newKernel.physical_address = CalculatePageAddress(&newPhysical);
        newKernel.virtual_address = virtual_address;

        alloc_page(&newKernel);
    }

    return;

    asm(
        "cli\n"
        "1:\n\t"
        "hlt\n"
        "jmp 1b\n"
        :::
    );
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