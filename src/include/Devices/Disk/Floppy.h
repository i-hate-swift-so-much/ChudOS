#pragma once

#include "LowLevel/IDT.h"
#include "Devices/IO.h"
#include "Devices/PCI.h"
#include "LowLevel/Timer.h"
#include "stdbool.h"

#define FLOPPY_CMD_

struct CHS_S{
    uint16_t Cylinder;
    uint16_t Head;
    uint16_t Sector;
}__attribute__((packed));

struct FLOPPY_Drive_Info_S{
    bool Initialized;
    uint64_t Last_Command_Timestamp; // seconds. should be cleared to zero when a command has successfully finished

    uint16_t Cylinder_MAX_Index;
    uint16_t Head_MAX_Index;
    uint16_t Sector_MAX_Index;

    uint16_t Cylinder_COUNT;
    uint16_t Head_COUNT;

    uint16_t Base_IO;
}__attribute__((packed));

enum FLOPPY_Controller_Registers{
    STATUS_REGISTER_A = 0x3F0, // READ ONLY
    STATUS_REGISTER_B = 0x3F1, // READ ONLY
    DIGITAL_OUTPUT_REGISTER = 0x3F2, // motor control, which floppy, and reset controls
    TAPE_DRIVE_REGISTER = 0x3F3, 
    MAIN_STATUS_REGISTER = 0x3F4, // READ ONLY | Contains the busy flags
    DATARATE_SELECT_REGISTER = 0x3F4, // WRITE ONLY
    DATA_FIFIO = 0x3F5,
    DIGITAL_INPUT_REGISTER = 0x3F7, // READ ONLY
    CONFIGURATION_CONTROL_REGISTER = 0x3F7 // WRITE ONLY
};

typedef struct FLOPPY_Drive_Info_S FLOPPY_Drive_Info;

typedef struct CHS_S CHS;

extern void floppy_drive_stub();

extern bool FLOPPY_FDC_Present;

int FLOPPY_Init_Drive(uint8_t drive_number);
int FLOPPY_Init_Controller();
void FLOPPY_Check_FDC();

CHS FLOPPY_LBA_To_CHS(uint64_t LBA);