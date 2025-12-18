#pragma once

#include <stdint.h>
#include <stdbool.h>

#include <IO.h>

#define PCI_CLASS_UND 0x00 // unclassified device, built before PCI 2.0 (could be VGA)
#define PCI_CLASS_STR 0x01 // mass storage controller (HDD/Floppy)
#define PCI_CLASS_NET 0x02 // network controller (Ethernet)
#define PCI_CLASS_DIS 0x03 // display controller (GPU)
#define PCI_CLASS_MUL 0x04 // multimedia device controller (Audio output/Video Capture)
#define PCI_CLASS_MEM 0x05 // memory controller, idek what it's for
#define PCI_CLASS_BRD 0x06 // bridge device
#define PCI_CLASS_SCC 0x07 // simple communication controllers
#define PCI_CLASS_BSP 0x08 // base system peripherals (PIC's)
#define PCI_CLASS_INP 0x09 // input devices (PS/2 keyboard and mouse)
#define PCI_CLASS_DOC 0xA0 // docking station
#define PCI_CLASS_PRO 0xB0 // processor (386, 486, Pentium, etc)
#define PCI_CLASS_SER 0xC0 // serial bus controller (Firewire, USB, etc)

struct PCI_Common_Header{
    uint16_t VendorID;
    uint16_t DeviceID;
    uint16_t Command;
    uint16_t Status;
    uint8_t RevisionID;
    uint8_t ProgIF;
    uint8_t SubClass;
    uint8_t ClassCode;
    uint8_t CacheLineSize;
    uint8_t LatencyTimer;
    uint8_t HeaderType;
    uint8_t BIST;
    bool MF;
}__attribute__((packed));

struct PCI_Header_0x0{
    PCI_Common_Header Common_Header;
    uint32_t BAR0;
    uint32_t BAR1;
    uint32_t BAR2;
    uint32_t BAR3;
    uint32_t BAR4;
    uint32_t BAR5;
    uint32_t Cardbus_CIS;
    uint16_t Subsystem_Vendor_ID;
    uint16_t Subsystem_ID;
    uint32_t ExpansionROM;
    uint16_t CapabilityPointer;
    uint16_t Reserved1;
    uint32_t Reserved2;
    uint8_t InterruptLine;
    uint8_t InterruptPIN;
    uint8_t MinGrant;
    uint8_t MaxLatency;
}__attribute__((packed));

struct PCI_Header_0x1{
    PCI_Common_Header Common_Header;
    uint32_t BAR0;
    uint32_t BAR1;
    uint8_t PrimaryBusNumber;
    uint8_t SecondaryBusNumber;
    uint8_t SubordinateBusNumber;
    uint8_t SecondaryLatencyTimer;
    uint8_t IOBase;
    uint8_t IOLimit;
    uint16_t SecondaryStatus;
    uint16_t MemoryBase;
    uint16_t MemoryLimit;
    uint16_t PrefetchMemoryBaseLower;
    uint16_t PrefetchMemoryLimitLower;
    uint32_t PrefetchBaseUpper;
    uint32_t PrefetchLimitUpper;
    uint16_t IOBaseUpper;
    uint16_t IOLimitUpper;
    uint16_t CapabilityPointer;
    uint16_t Reserved1;
    uint32_t ExpansionROM;
    uint8_t InterruptLine;
    uint8_t InterruptPIN;
    uint16_t BridgeControl;
}__attribute__((packed));

struct PCI_Device{
    uint8_t Header;
    PCI_Header_0x0 Header0;
    PCI_Header_0x1 Header1;
}__attribute__((packed));

extern PCI_Device Main_SATA_Controller;

uint32_t PCI_CreateConfigAddress(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
PCI_Common_Header PCI_ReadCommonHeader(uint8_t bus, uint8_t slot);