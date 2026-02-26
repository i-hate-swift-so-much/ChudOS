#pragma once

#include "Devices/Disk/Floppy.h"

#include "stdint.h"
#include "stddef.h"

enum GemFS_DriveIDs{
    F0, // Floppy drive 0
    F1, // Floppy drive 1
    F2, // Floppy drive 2
    F3, // Floppy drive 3
    H0, // Hard Drive 0
    H1, // Hard Drive 1
    H2, // Hard Drive 2
    H3, // Hard Drive 3
};

struct GemFS_Entry{
    uint64_t Start;
    uint64_t Size;
    uint64_t Index;
    uint64_t Parent_Index;
    uint64_t Sibling_Index;
    uint64_t Next_Index_Start;
    uint8_t Flags;
    uint8_t ReadingMask;
    char Name[128];
}__attribute__((packed));

struct GemFS_Main{
    char check[4]; // ‘GEMH’
    uint64_t Bitmap_LBA;
    uint16_t Block_Size; // 4096 for hdd/sdd, 512 for floppy
    struct GemFS_Entry Entry;
}__attribute__((packed));

void GemFS_LoadPartitionTable(enum GemFS_DriveIDs DriveID);

void GemFS_Init(enum GemFS_DriveIDs DriveID, uint8_t Partition);