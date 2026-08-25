#pragma once

#include "Devices/PCI.h"
#include "Devices/IO.h"
#include "LowLevel/Timer.h"
#include "LowLevel/IDT.h"

#include "stdbool.h"

// real coding for this driver started
// dec 29, 2025.
// note to self, AHCI specification includes 
// the definition for the HBA's PCI header
// chapter 2 section 1

// note to self 2
// DMA means direct memory access, must be enabled with
// a FIS_TYPE_DMA_ACT FIS

// AHCI uses a packet system kind of like ethernet
// called FIS (Frame Information Structure) which is

// types of FIS packets, defined by their sizes
typedef enum{
    FIS_TYPE_REG_H2D =      0x27,  // register host to device, used for transferring the shadow registers to the device. this is how ATA commands are issued
    FIS_TYPE_REG_D2H =      0x34,  // register device to host, used for responding to H2D's
    FIS_TYPE_DMA_ACT =      0x36,  // DMA activate, device to host
    FIS_TYPE_DMA_SETUP =    0x41,  // DMA setup, bidirectional
    FIS_TYPE_DATA =         0x46,  // Data FIS, bidirectional
    FIS_TYPE_BIST =         0x58,  // Built in Self Test FIS, bidirectional
    FIS_TYPE_PIO_SETUP =    0x5F,  // PIO setup, device to host
    FIS_TYPE_DEV_BITS =     0xA1   // Set device bits, device to host
}FIS_TYPE;

typedef enum{
    NOP = 0x0,
    CFA_REQ_EXT_ERRC = 0x3,
    DATA_MAN = 0x6,
    DATA_MAN_XL = 0x7,
    DEV_RESET = 0x8,
    REQ_SENSE_DEXT = 0x0B,
    RECALIBRATE_H = 0x10,
    READ_DMA_EXT = 0x25,
    READ_DMA_QUEUED_EXT = 0x26,
    WRITE_DMA_EXT = 0x35,
    WRITE_DMA_QUEUED_EXT = 0x36,
    IDENTIFY = 0xEC
}ATA_COMMAND;

// just learned this!
// putting a colon (:) and number after a struct
// member definition makes it bit specific, eg
// uint8_t foo:1; only takes up 1 bit

struct FIS_REG_H2D_S{
    // DWORD 0
    uint8_t features_l;
    uint8_t command;
    uint8_t C:1; // if 1, transfer is due to update of the Command register. if 0, transfer is due to update of the Control register.
    uint8_t rsv0:3; // reserved, 0
    uint8_t pm_port:4; // if the device is attached by an port multiplier, then this specifies which port the FIS should be directed to.
    uint8_t FIS_Type; // FIS_TYPE_REG_H2D

    // DWORD 1
    uint8_t device;
    uint32_t LBA_L:24; // lower 24 bits of the starting LBA

    // DWORD 2
    uint8_t features_h;
    uint32_t LBA_H:24; // upper 24 bits of the starting LBA

    // DWORD 3
    uint8_t control;
    uint8_t ICC; // tells the device about a time limit for whatever command
    uint16_t LBA_Count; // sector count

    // DWORD 4
    uint64_t auxillary;
}__attribute__((packed));

typedef struct FIS_REG_H2D_S FIS_REG_H2D;

struct AHCI_PORT_S{
    uint64_t Command_List_BAR;
    uint64_t FIS_BAR;
    uint32_t Interrupt_Status;
    uint32_t Interrupt_Enable;
    uint32_t Command;
    uint32_t Reserved_1;
    uint32_t Task_File_Data;
    uint32_t Signature;
    uint32_t SATA_Status;
    uint32_t SATA_Control;
    uint32_t SATA_Error;
    uint32_t SATA_Active;
    uint32_t Command_Issue;
    uint32_t SATA_Notification;
    uint32_t FIS_Based_Switch_Control;
    uint32_t Reserved_2[11];
    uint32_t vendor[4];
}__attribute__((packed));

typedef struct AHCI_PORT_S AHCI_PORT;

struct AHCI_HOST_CAPABILITES{
    uint8_t NP : 4; // number of ports supported by the HBA
    bool SXS : 1; // supports external sata
    bool EMS : 1; 
    bool CCS : 1;
    uint8_t NCS : 4; // number of command slots
    bool PSC : 1;
    bool SSC : 1;
    bool PMQ : 1;
    bool FBSS : 1;
    bool SPM : 1;
    bool SAM : 1;
    uint8_t ISS : 4; // speed (0b0001 = 1.5Gbps 0b0010 = 3 Gbps 0b0011 = 6 Gbps)
    bool SCLO : 1;
    bool SAL : 1; // supports activity LED
    bool SALP : 1;
    bool SSS : 1;
    bool SMPS : 1;
    bool SSNTF : 1;
    bool SNCQ : 1;
    bool S64A : 1; // supports 64 bit addressing
} __attribute__((packed));

struct AHCI_GEN_HOST_CONTROL{
    struct AHCI_HOST_CAPABILITES CAP;
} __attribute__((packed));

struct AHCI_BOHC{
    bool BOS : 1;
    bool OOS : 1;
    bool SOOE : 1;
    bool BB : 1;
    uint32_t : 28;
}__attribute__((packed));

struct AHCI_MMIO{
    // generic host control
    uint32_t Host_Capabilities;
    uint32_t Global_Host_Control;
    uint32_t Interrupt_Status; // bitmap of every port, 1 means it has fired, set to 0 for EOI
    uint32_t Port_Implemented;
    uint32_t Version;
    uint32_t Command_Completion_CC;
    uint32_t Command_Completion_CP;
    uint32_t Encloser_Management_Location;
    uint32_t Encloser_Managament_Control;
    uint32_t Extended_Host_Capabilities;
    struct AHCI_BOHC BOHC;

    // reserved
	uint8_t  Reserved[0xA0-0x2C];
	uint8_t  vendor[0x100-0xA0];

    // ports
    AHCI_PORT Ports[4]; // min 1 max 32
} __attribute__((packed));

// AHCI 4.2.2
struct AHCI_COMMAND_HEADER_S{
    // DWORD 0
    uint8_t CFL:5; // length of command fis measured in 32 bits
    uint8_t A:1; // ATAPI
    uint8_t W:1; // 1 = device->memory, 0 = memory->device
    uint8_t P:1; // Prefetchable

    uint8_t R:1; // Reset
    uint8_t B:1; // BIST
    uint8_t C:1; // when an R_OK is sent and this bit is set, the port will automatically clear its busy bit
    uint8_t Res1:1;

    uint8_t PMP:4; // 0
    uint16_t PRDTL; // length of the PRDT in entries
    // the PRDT is used to address DMA over non contiguous physical memory blocks.

    // DWORD 1
    volatile
    uint32_t PRD_Byte_Count;

    // DWORD 2, 3
    uint32_t CTBA0;
    uint32_t CTBA_U0;

    // DWORD 4
    uint32_t reserved[4];
}__attribute__((packed));

struct AHCI_Physical_Region_Descriptor_S{
    uint32_t DBA_L;
    uint32_t DBA_U;
    uint32_t reserved1;
    
    uint32_t Byte_Count:22;		// 4M max
	uint32_t reserved2:9;		// Reserved
	uint32_t i:1;		// Interrupt on completion
}__attribute__((packed));


struct AHCI_CMD_Table_S{
    uint8_t cfis[64];

    uint8_t acmd[16];

    uint8_t reserved[48];

    struct AHCI_Physical_Region_Descriptor* prdt_entry;
}__attribute__((packed));

typedef struct AHCI_COMMAND_HEADER_S AHCI_COMMAND_HEADER;
typedef struct AHCI_CMD_Table_S AHCI_CMD_Table;

extern bool AHCI_Ownership;

extern void ahci_interrupt_stub();
void AHCI_Init(PCI_Device* device);