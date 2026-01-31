#pragma once

#include "Devices/PCI.h"
#include "Devices/IO.h"
#include "LowLevel/Timer.h"
#include "LowLevel/IDT.h"

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
#define FIS_TYPE_REG_H2D 0x27 // register host to device, used for transferring the shadow registers to the device. this is how ATA commands are issued
#define FIS_TYPE_REG_D2H 0x34 // register device to host, used for responding to H2D's
#define FIS_TYPE_DMA_ACT 0x36 // DMA activate, device to host
#define FIS_TYPE_DMA_SETUP 0x41 // DMA setup, bidirectional
#define FIS_TYPE_DATA 0x46 // Data FIS, bidirectional
#define FIS_TYPE_BIST 0x58 // Built in Self Test FIS, bidirectional
#define FIS_TYPE_PIO_SETUP 0x5F // PIO setup, device to host
#define FIS_TYPE_DEV_BITS 0xA1 // Set device bits, device to host

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

struct AHCI_MMIO_S{
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
    uint32_t Handoff_Status;

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
    uint8_t W:1; // 1 = write, 0 = read
    uint8_t P:1; // Prefetchable

    uint8_t R:1; // Reset
    uint8_t B:1; // BIST
    uint8_t C:1; // when an R_OK is sent and this bit is set, the port will automatically clear its busy bit
    uint8_t Res1:1;

    uint8_t PMP:4; // 0
    uint16_t PRDTL; // length of the PRDT in entries

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

typedef struct AHCI_Physical_Region_Descriptor_S AHCI_Physical_Region_Descriptor;

struct AHCI_CMD_Table_S{
    uint8_t cfis[64];

    uint8_t acmd[16];

    uint8_t reserved[48];

    AHCI_Physical_Region_Descriptor prdt_entry[32];
}__attribute__((packed));

typedef struct AHCI_MMIO_S AHCI_MMIO;
typedef struct AHCI_COMMAND_HEADER_S AHCI_COMMAND_HEADER;
typedef struct AHCI_CMD_Table_S AHCI_CMD_Table;

extern bool AHCI_Ownership;

extern void ahci_interrupt_stub();
void AHCI_Init(PCI_Device* device);