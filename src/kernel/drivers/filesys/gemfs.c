#include "filesys/gemfs.h"

struct GemFS_DriveData Drives[16];

void GemFS_FormatPartition(enum GemFS_DriveIDs DriveID, uint8_t Partition){

}

/**
 * @brief Formats a main entry and basic structure into a drives partition. The partition table must already be set up.
 */
void GemFS_Format(enum GemFS_DriveIDs DriveID, uint8_t Partition){
    struct GemFS_DriveData Drive = Drives[DriveID];
    
    struct GemFS_Main Main_Entry;
}

void GemFS_ReadEntry(enum GemFS_DriveIDs DriveID, uint8_t Partition, uint64_t Index){

}

void GemFS_Init(enum GemFS_DriveIDs DriveID, uint8_t Partition){
    struct GemFS_DriveData Drive = Drives[DriveID];
    
    GemFS_LoadPartitionTable(DriveID);
}

void GemFS_DumpPartition(enum GemFS_DriveIDs DriveID, uint8_t Partition){
    struct GemFS_MBRPartition Table = Drives[DriveID].PartitionTable[Partition];
    SetTextColor(LCYAN, BLACK);
    printf("DRIVE PARTITION DUMP ID: %x\n\t", DriveID);
    printf("STATUS: %x\n\t", Table.Status);
    printf("TYPE: %x\n\t", Table.Type);
    printf("LBA_START: %x\n\t", Table.LBA_Start);
    printf("SECTOR_COUNT: %x\n", Table.Sector_Count);
    SetTextColor(WHITE, BLACK);
}

void GemFS_DumpDriveData(enum GemFS_DriveIDs DriveID){
    struct GemFS_DriveData Drive = Drives[DriveID];
    SetTextColor(LCYAN, BLACK);
    printf("DRIVE DATA DUMP ID: %x\n\t", Drive.DriveID);
    printf("IS_GEMFS: %i\n\t", Drive.Is_GemFS);
    printf("PARTITION TABLE:\n\t\t");
}

uint8_t SectorBuffer[512];

void GemFS_LoadPartitionTable(enum GemFS_DriveIDs DriveID){
    if(DriveID < H0){
        // read the MBR from the floppy
        FLOPPY_Read_LBA(DriveID, 0, (uint64_t)SectorBuffer, 1);
        memcpy(&Drives[DriveID].PartitionTable, &SectorBuffer[446], sizeof(Drives[DriveID].PartitionTable)); // copy the partition table
    }else{

    }
}