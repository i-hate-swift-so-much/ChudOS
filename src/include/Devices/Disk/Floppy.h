#pragma once

#include "LowLevel/IDT.h"
#include "Devices/IO.h"
#include "Devices/PCI.h"
#include "LowLevel/Timer.h"
#include "LowLevel/Memory.h"
#include "stdbool.h"

struct FLOPPY_Info_Struct_S{
    uint8_t drive_count;
    uint16_t cylinder_max;
    uint8_t head_max;
    uint8_t sector_max;
    uint8_t head_count;
}__attribute__((packed));

typedef struct FLOPPY_Info_Struct_S FLOPPY_Info_Struct;

struct CHS_S{
    uint16_t Cylinder;
    uint16_t Head;
    uint16_t Sector;
}__attribute__((packed));

struct FLOPPY_Drive_Info_S{
    bool Initialized; // offset 0

    uint16_t Cylinder_MAX_Index; // offset 1
    uint8_t Head_MAX_Index; // offset 3
    uint8_t Sector_MAX_Index; // offset 4

    uint8_t Head_COUNT; // offset 5

    uint16_t Base_IO; // offset 6
}__attribute__((packed));

enum FLOPPY_Controller_Registers{
    STATUS_REGISTER_A = 0x3F0, // READ ONLY
    STATUS_REGISTER_B = 0x3F1, // READ ONLY
    DIGITAL_OUTPUT_REGISTER = 0x3F2, // motor control, which floppy, and reset controls
    TAPE_DRIVE_REGISTER = 0x3F3, 
    MAIN_STATUS_REGISTER = 0x3F4, // READ ONLY | Contains the busy flags
    DATARATE_SELECT_REGISTER = 0x3F4, // WRITE ONLY
    DATA_FIFO = 0x3F5,
    DIGITAL_INPUT_REGISTER = 0x3F7, // READ ONLY
    CONFIGURATION_CONTROL_REGISTER = 0x3F7 // WRITE ONLY
};

enum FLOPPPY_Commands{
   READ_TRACK =                 2,	// generates IRQ6
   SPECIFY =                    3,      // * set drive parameters
   SENSE_DRIVE_STATUS =         4,
   WRITE_DATA =                 5,      // * write to the disk
   READ_DATA =                  6,      // * read from the disk
   RECALIBRATE =                7,      // * seek to cylinder 0
   SENSE_INTERRUPT =            8,      // * ack IRQ6, get status of last command
   WRITE_DELETED_DATA =         9,
   READ_ID =                    10,	// generates IRQ6
   READ_DELETED_DATA =          12,
   FORMAT_TRACK =               13,     // *
   DUMPREG =                    14,
   SEEK =                       15,     // * seek both heads to cylinder X
   VERSION =                    16,	// * used during initialization, once
   SCAN_EQUAL =                 17,
   PERPENDICULAR_MODE =         18,	// * used during initialization, once, maybe
   CONFIGURE =                  19,     // * set controller parameters
   LOCK =                       20,     // * protect controller params from a reset
   VERIFY =                     22,
   SCAN_LOW_OR_EQUAL =          25,
   SCAN_HIGH_OR_EQUAL =         29
};

typedef struct FLOPPY_Drive_Info_S FLOPPY_Drive_Info;

typedef struct CHS_S CHS;

extern void floppy_drive_stub();

extern bool FLOPPY_FDC_Present;

int FLOPPY_Init_Drive(uint8_t drive_number);
int FLOPPY_Init_Controller();
void FLOPPY_Check_FDC();

CHS FLOPPY_LBA_To_CHS(uint64_t LBA);