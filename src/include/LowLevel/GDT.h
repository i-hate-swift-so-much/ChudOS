#pragma once

#include "stdint.h"
#include "stddef.h"
#include "stdbool.h"

#include "LowLevel/Memory.h"

#define GDT_Access_Kernel_Code 0b10011010
#define GDT_Access_Kernel_Data 0b10010010

#define GDT_Access_User_Code 0b11111010
#define GDT_Access_User_Data 0b11110010

#define GDT_Access_TSS 0b10001001

#define GDT_Flags_Data 0b0000
#define GDT_Flags_Code 0b0010

struct GDTR{
    uint16_t limit;
    uint64_t base;
}__attribute__((packed));

struct GDT_Entry{
    uint16_t limit;
    uint16_t base_0;
    uint8_t base_1;
    uint8_t access;
    uint8_t flags_limits;
    uint8_t base_2;
}__attribute__((packed));

struct TSS_GDT{
    uint16_t limit;
    uint16_t base_0;
    uint8_t base_1;
    uint8_t access;
    uint8_t flags_limits;
    uint8_t base_2;
    uint32_t base_3;
    uint32_t reserved;
}__attribute__((packed));

extern volatile struct TSS ActiveTSS;

void SetGDTEntry(uint32_t Base, uint32_t Limit, uint8_t Flags, uint8_t Access, uint16_t Offset);
void SetGDTSystemEntry(uint64_t Base, uint32_t Limit, uint8_t Flags, uint8_t Access, uint16_t Offset);
void SetActiveTSS(uint64_t RSP0, uint64_t RSP1, uint64_t RSP2, uint64_t IST1, uint64_t IST2, uint64_t IST3, uint64_t IST4, uint64_t IST5, uint64_t IST6, uint64_t IST7, uint16_t IO_Base);
void UpdateActiveTSS(struct TSS* NewTSS);
void LoadGDT();