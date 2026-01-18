#include "GDT.h"

GDTR Main_GDTR;
GDT_Entry Main_GDT[10];

void LoadGDT(){
    Main_GDTR.base = (uint64_t)&Main_GDT;
    Main_GDTR.limit = sizeof(Main_GDT) - 1;

    asm volatile(
        "lgdt %0"
        : 
        : "m"(Main_GDTR) 
        : "memory"
    );

    return;

    asm volatile(
        "pushq $0x08\n"     // code segment selector
        "lea 1f(%rip), %rax\n" // target RIP
        "pushq %rax\n"
        "lretq\n"           // far return to update CS
        "1:\n"
    );
}

void SetGDTEntry(size_t offset, GDT_Entry entry){
    *(Main_GDT+offset) = entry;
}

GDT_Entry KernelGDTCode;
GDT_Entry KernelGDTData;
GDT_Entry UserGDTCode;
GDT_Entry UserGDTData;
TSS_GDT Main_TSS_GDT;

void SetupBasicGDT(){
    memset(Main_GDT, 0, 80);

    KernelGDTCode.base0 = 0;
    KernelGDTCode.base1 = 0;
    KernelGDTCode.base2 = 0;
    KernelGDTCode.limit0 = 0;
    KernelGDTCode.access = 0b10011010;
    KernelGDTCode.flags = 0b10100000;

    SetGDTEntry(0x08, KernelGDTCode);

    KernelGDTData.base0 = 0;
    KernelGDTData.base1 = 0;
    KernelGDTData.base2 = 0;
    KernelGDTData.limit0 = 0;
    KernelGDTData.access = 0b10010010;
    KernelGDTData.flags = 0b10100000;

    SetGDTEntry(0x10, KernelGDTData);

    UserGDTCode.base0 = 0;
    UserGDTCode.base1 = 0;
    UserGDTCode.base2 = 0;
    UserGDTCode.limit0 = 0xFFFF;
    UserGDTCode.access = 0b11111010;
    UserGDTCode.flags = 0b10101111; 

    //SetGDTEntry(3, UserGDTCode);

    UserGDTData.base0 = 0;
    UserGDTData.base1 = 0;
    UserGDTData.base2 = 0;
    UserGDTData.limit0 = 0xFFFF;
    UserGDTData.access = 0b11101010;
    UserGDTData.flags = 0b10101111;

    //SetGDTEntry(4, UserGDTData);

    LoadGDT();
}