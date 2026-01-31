#pragma once

#include "stdint.h"
#include "stddef.h"
#include "stdbool.h"

#include "LowLevel/Memory.h"

struct GDTR_S{
    uint16_t limit;
    uint64_t base;
}__attribute__((packed));

struct GDT_Entry_S{
    uint16_t limit0;
    uint16_t base0;
    uint8_t base1;
    uint8_t access;
    uint8_t flags;
    uint8_t base2;
}__attribute__((packed));

struct TSS_S{
    uint32_t reserved0;
    uint64_t rsp0;
    uint64_t rsp1;
    uint64_t rsp2;
    uint64_t reserved1;
    uint64_t ist1;
    uint64_t ist2;
    uint64_t ist3;
    uint64_t ist4;
    uint64_t ist5;
    uint64_t ist6;
    uint64_t ist7;
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t io_map_base;
}__attribute__((packed));

struct TSS_GDT_S{
    uint16_t limit0;
    uint16_t base0;
    uint8_t base1;
    uint8_t type:4;
    uint8_t reserved0:1;
    uint8_t DPL:2;
    uint8_t present:1;
    uint8_t limit1:4;
    uint8_t avl:1;
    uint8_t reserved1:2;
    uint8_t granularity:1;
    uint8_t base2;
    uint32_t base3;
    uint32_t reserved2;
}__attribute__((packed));

typedef struct TSS_GDT_S TSS_GDT;
typedef struct TSS_S TSS;

typedef struct GDT_Entry_S GDT_Entry;
typedef struct GDTR_S GDTR;

void SetGDTEntry(size_t offset, GDT_Entry entry);
void LoadGDT();
void SetupBasicGDT();