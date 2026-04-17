#include "LowLevel/GDT.h"

volatile struct GDTR Main_GDTR;
uint8_t Main_GDT[64];

volatile struct TSS ActiveTSS;

void LoadGDT(){
    Main_GDTR.base = (uint64_t)&Main_GDT;
    Main_GDTR.limit = sizeof(Main_GDT) - 1;

    asm volatile(
        "lgdtq %0"
        : 
        : "m"(Main_GDTR) 
        : "memory"
    );

    return;
}

void SetGDTEntry(uint32_t Base, uint32_t Limit, uint8_t Flags, uint8_t Access, uint16_t Offset){
    struct GDT_Entry* entry = (struct GDT_Entry*)(Main_GDT+Offset);
    entry->base_0 = Base & 0xffff;
    entry->base_1 = (Base >> 16) & 0xff;
    entry->base_2 = (Base >> 24) & 0xff;
    entry->access = Access;
    entry->flags_limits = Flags << 4 | ((Limit >> 16) & 0xf);
    entry->limit = Limit & 0xff;
}

void SetGDTSystemEntry(uint64_t Base, uint32_t Limit, uint8_t Flags, uint8_t Access, uint16_t Offset){
    struct TSS_GDT* entry = (struct TSS_GDT*)(Main_GDT+Offset);
    entry->base_0 = Base & 0xffff;
    entry->base_1 = (Base >> 16) & 0xff;
    entry->base_2 = (Base >> 24) & 0xff;
    entry->access = Access;
    entry->flags_limits = Flags << 4 | ((Limit >> 16) & 0xf);
    entry->limit = Limit & 0xff;
    entry->base_3 = (Base >> 32) & 0xffffffff;
    entry->reserved = 0;
}

void SetActiveTSS(uint64_t RSP0, uint64_t RSP1, uint64_t RSP2, uint64_t IST1, uint64_t IST2, uint64_t IST3, uint64_t IST4, uint64_t IST5, uint64_t IST6, uint64_t IST7, uint16_t IO_Base){
    memset(&ActiveTSS, 0, sizeof(ActiveTSS));
    ActiveTSS.rsp0 = RSP0;
    ActiveTSS.rsp1 = RSP1;
    ActiveTSS.rsp2 = RSP2;
    ActiveTSS.ist1 = IST1;
    ActiveTSS.ist2 = IST2;
    ActiveTSS.ist3 = IST3;
    ActiveTSS.ist4 = IST4;
    ActiveTSS.ist5 = IST5;
    ActiveTSS.ist6 = IST6;
    ActiveTSS.ist7 = IST7;
    ActiveTSS.io_map_base = IO_Base;
}

void UpdateActiveTSS(struct TSS* NewTSS){
    memcpy(&ActiveTSS, NewTSS, sizeof(struct TSS));
}