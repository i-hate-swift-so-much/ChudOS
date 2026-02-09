#include "ErrorHandling/Exceptions.h"

void HandlePageFault(InterruptRegistersError* regs){
    print("Page fault\n", 0);

    PageEntries newPhysical = FindNextFreePhysical();
    uint64_t virtual_address = regs->cr2;

    void* confirmation;

    if(virtual_address > VIRTUAL_MEMORY_BARRIER){
        print("User\n", 0);
        
        PageDetails newUser;
        newUser.flags.flags = USER_FLAGS;
        newUser.flags.Execute_Disable = false;
        newUser.physical_address = CalculatePageAddress(&newPhysical);
        newUser.virtual_address = virtual_address;

        alloc_page(&newUser);
    }else{
        print("Kernel\n", 0);
        PageDetails newKernel;
        newKernel.flags.flags = KERNEL_FLAGS;
        newKernel.flags.Execute_Disable = false;
        newKernel.physical_address = CalculatePageAddress(&newPhysical);
        newKernel.virtual_address = virtual_address;

        alloc_page(&newKernel);
    }

    return;

    if(confirmation == NULL){
        print("NULL\n", 0);
    }else{
        char pointer_char[68];
        int_to_char_array_hex((uintptr_t)confirmation, pointer_char, sizeof(pointer_char), 0);
        print(pointer_char, 0);
        print("\n", 0);
        
        PageEntries extract = ExtractPageEntries((uintptr_t)confirmation);
        int_to_char_array_binary((uint64_t)*CalculatePagePhysicalEntryAddress(&extract), pointer_char, sizeof(pointer_char), 0);
        print(pointer_char, 0);
        print("\n", 0);
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
    return;
}