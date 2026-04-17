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

struct GemFS_MBRPartition{
    uint8_t Status; // 0x80 = bootable, 0x00 = not bootable
    uint16_t Reserved_0; // Reserved_0, 1, 2, and 3 are just the CHS but we don't care about that
    uint8_t Reserved_1;
    uint8_t Type;
    uint16_t Reserved_2;
    uint8_t Reserved_3;
    uint32_t LBA_Start;
    uint32_t Sector_Count;
}__attribute__((packed));

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
    uint16_t Block_Size; // 4096 for hdd/sdd, 512 for floppy
    struct GemFS_Entry Entry;
}__attribute__((packed));

struct GemFS_DriveData{
    uint16_t DriveID;
    struct GemFS_MBRPartition PartitionTable[4];
    struct GemFS_Main Main_Entries[4];
    bool Is_GemFS[4];
};

extern struct GemFS_DriveData Drives[16];

uint64_t GemFS_LBAToBlock(enum GemFS_DriveIDs DriveID, uint8_t Partition, uint64_t LBA);
uint64_t GemFS_BlockToLBA(enum GemFS_DriveIDs DriveID, uint8_t Partition, uint64_t Block);
void GemFS_GetFBB(enum GemFS_DriveIDs DriveID, uint8_t Partition);
bool GemFS_FBB_GetBlock(enum GemFS_DriveIDs DriveID, uint8_t Partition, uint64_t Block);
void GemFS_FBB_SetBlock(enum GemFS_DriveIDs DriveID, uint8_t Partition, uint64_t Block);

void GemFS_CreateEntry(enum GemFS_DriveIDs DriveID, uint8_t Partititon, uint64_t Index, uint8_t flags);
void GemFS_WriteEntry(enum GemFS_DriveIDs DriveID, uint8_t Partition, uint64_t Index, struct GemFS_Entry entry);
struct GemFS_Entry GemFS_ReadEntry(enum GemFS_DriveIDs DriveID, uint8_t Partition, uint64_t Index);
uint64_t GemFS_GetEntryBlock(enum GemFS_DriveIDs DriveID, uint8_t Partition, uint64_t Index);
uint64_t GemFS_FindFreeEntry(enum GemFS_DriveIDs DriveID, uint8_t Partition);
struct GemFS_Main GemFS_ReadMainEntry(enum GemFS_DriveIDs DriveID, uint8_t Partition);
uint64_t GemFS_FindFreeBlock(enum GemFS_DriveIDs DriveID, uint8_t Partition);

void GemFS_DumpPartition(enum GemFS_DriveIDs DriveID, uint8_t Partition);
void GemFS_DumpDriveData(enum GemFS_DriveIDs DriveID);

void GemFS_FormatPartition(enum GemFS_DriveIDs DriveID, uint8_t Partition);
void GemFS_LoadPartitionTable(enum GemFS_DriveIDs DriveID);

uint64_t GemFS_mkdir(enum GemFS_DriveIDs DriveID, uint8_t Partition, char* name, size_t name_len, uint8_t flags, uint64_t ParentIndex);

uint64_t GemFS_CreateFile(enum GemFS_DriveIDs DriveID, uint8_t Partition, char* name, size_t name_len, uint8_t flags, uint64_t ParentIndex, uint64_t Size);
void GemFS_WriteFile(void* buffer, size_t buffer_size, enum GemFS_DriveIDs DriveID, uint8_t Partition, uint64_t Index);
void GemFS_ReadFile(enum GemFS_DriveIDs DriveID, uint8_t Partition, uint64_t index, void* buffer, uint64_t Blocks);

uint64_t GemFS_Find_Index_By_Name(enum GemFS_DriveIDs DriveID, uint8_t Partition, char* name, uint64_t Parent_Index);

void GemFS_DumpEntryData(struct GemFS_Entry entry);

uint64_t GemFS_Directory_to_Index(enum GemFS_DriveIDs DriveID, uint8_t Partition, char* directory);

void GemFS_Init(enum GemFS_DriveIDs DriveID);