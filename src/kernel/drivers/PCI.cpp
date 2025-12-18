#include "PCI.h"

#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA 0xCFC

#define BUS_COUNT

PCI_Device Devices[256];

uint32_t PCI_CreateConfigAddress(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset){
    uint32_t lbus = (uint32_t)bus;
    uint32_t lslot = (uint32_t)slot;
    uint32_t lfunc = (uint32_t)func;

    uint32_t address = 
        (uint32_t)((lbus << 16) | 
        (lslot << 1) | 
        (lfunc << 8) |
        (offset & 0xFC) | 
        ((uint32_t)0x80000000));

    return address;
}

PCI_Common_Header PCI_ReadCommonHeader(uint8_t bus, uint8_t slot){
    PCI_Common_Header ret;
    uint32_t temp;

    uint8_t offset = 0;
    
    // Get VendorID
    uint32_t address = PCI_CreateConfigAddress(bus, slot, 0, offset);
    outl(PCI_CONFIG_ADDRESS, address);
    temp = inl(PCI_CONFIG_DATA);
    ret.VendorID = (uint16_t)((temp >> 0) & 0xFFFF);
    ret.DeviceID = (uint16_t)((temp >> 16) & 0xFFFF);
    offset += 0x4;
    
    address = PCI_CreateConfigAddress(bus, slot, 0, offset);
    outl(PCI_CONFIG_ADDRESS, address);
    temp = inl(PCI_CONFIG_DATA);
    ret.Command = (uint16_t)((temp >> 0) & 0xFFFF);
    ret.Status = (uint16_t)((temp >> 16) & 0xFFFF);
    offset += 0x4;

    address = PCI_CreateConfigAddress(bus, slot, 0, offset);
    outl(PCI_CONFIG_ADDRESS, address);
    temp = inl(PCI_CONFIG_DATA);
    ret.RevisionID = (temp >> 0) & 0xFF;
    ret.ProgIF = (temp >> 8) & 0xFF;
    ret.SubClass = (temp >> 16) & 0xFF;
    ret.ClassCode = (temp >> 24) & 0xFF;
    offset += 0x4;

    address = PCI_CreateConfigAddress(bus, slot, 0, offset);
    outl(PCI_CONFIG_ADDRESS, address);
    temp = inl(PCI_CONFIG_DATA);
    ret.CacheLineSize = (temp >> 0) & 0xFF;
    ret.LatencyTimer = (temp >> 8) & 0xFF;
    ret.HeaderType = (temp >> 16) & 0xFF;
    ret.BIST = (temp >> 24) & 0xFF;

    return ret;
}

PCI_Header_0x0 PCI_ReadHeader0(PCI_Common_Header common, uint8_t bus, uint8_t slot){
    PCI_Header_0x0 ret;
    uint32_t temp;

    ret.Common_Header = common;

    uint16_t offset = 0x10;

    uint32_t address = PCI_CreateConfigAddress(bus, slot, 0, offset);
    outl(PCI_CONFIG_ADDRESS, address);
    ret.BAR0 = inl(PCI_CONFIG_DATA);
    offset += 4;

    address = PCI_CreateConfigAddress(bus, slot, 0, offset);
    outl(PCI_CONFIG_ADDRESS, address);
    ret.BAR1 = inl(PCI_CONFIG_DATA);
    offset += 4;

    address = PCI_CreateConfigAddress(bus, slot, 0, offset);
    outl(PCI_CONFIG_ADDRESS, address);
    ret.BAR2 = inl(PCI_CONFIG_DATA);
    offset += 4;

    address = PCI_CreateConfigAddress(bus, slot, 0, offset);
    outl(PCI_CONFIG_ADDRESS, address);
    ret.BAR3 = inl(PCI_CONFIG_DATA);
    offset += 4;

    address = PCI_CreateConfigAddress(bus, slot, 0, offset);
    outl(PCI_CONFIG_ADDRESS, address);
    ret.BAR4 = inl(PCI_CONFIG_DATA);
    offset += 4;

    address = PCI_CreateConfigAddress(bus, slot, 0, offset);
    outl(PCI_CONFIG_ADDRESS, address);
    ret.BAR5 = inl(PCI_CONFIG_DATA);
    offset += 4;

    address = PCI_CreateConfigAddress(bus, slot, 0, offset);
    outl(PCI_CONFIG_ADDRESS, address);
    ret.Cardbus_CIS = inl(PCI_CONFIG_DATA);
    offset += 4;

    address = PCI_CreateConfigAddress(bus, slot, 0, offset);
    outl(PCI_CONFIG_ADDRESS, address);
    temp = inl(PCI_CONFIG_DATA);
    ret.Subsystem_Vendor_ID = (uint16_t)((temp >> 0) & 0xFFFF);
    ret.Subsystem_ID = (uint16_t)((temp >> 16) & 0xFFFF);
    offset += 4;

    address = PCI_CreateConfigAddress(bus, slot, 0, offset);
    outl(PCI_CONFIG_ADDRESS, address);
    ret.ExpansionROM = inl(PCI_CONFIG_DATA);
    offset += 4;

    address = PCI_CreateConfigAddress(bus, slot, 0, offset);
    outl(PCI_CONFIG_ADDRESS, address);
    temp = inl(PCI_CONFIG_DATA);
    ret.CapabilityPointer = (uint16_t)((temp >> 0) & 0xFF);
    offset += 8;

    address = PCI_CreateConfigAddress(bus, slot, 0, offset);
    outl(PCI_CONFIG_ADDRESS, address);
    temp = inl(PCI_CONFIG_DATA);
    ret.InterruptLine = (uint8_t)((temp >> 0) & 0xFF);
    ret.InterruptPIN = (uint8_t)((temp >> 8) & 0xFF);
    ret.MinGrant = (uint8_t)((temp >> 16) & 0xFF);
    ret.MaxLatency = (uint8_t)((temp >> 24) & 0xFF);

    return ret;
}

PCI_Header_0x1 PCI_ReadHeader1(PCI_Common_Header common, uint8_t bus, uint8_t slot){
    PCI_Header_0x1 ret;
    uint32_t temp;

    ret.Common_Header = common;

    uint16_t offset = 0x10;

    uint32_t address = PCI_CreateConfigAddress(bus, slot, 0, offset);
    outl(PCI_CONFIG_ADDRESS, address);
    ret.BAR0 = inl(PCI_CONFIG_DATA);
    offset += 4;

    address = PCI_CreateConfigAddress(bus, slot, 0, offset);
    outl(PCI_CONFIG_ADDRESS, address);
    ret.BAR1 = inl(PCI_CONFIG_DATA);
    offset += 4;

    address = PCI_CreateConfigAddress(bus, slot, 0, offset);
    outl(PCI_CONFIG_ADDRESS, address);
    temp = inl(PCI_CONFIG_DATA);
    ret.PrimaryBusNumber = (uint8_t)((temp >> 0) & 0xFF);
    ret.SecondaryBusNumber = (uint8_t)((temp >> 8) & 0xFF);
    ret.SubordinateBusNumber = (uint8_t)((temp >> 16) & 0xFF);
    ret.SecondaryLatencyTimer = (uint8_t)((temp >> 24) & 0xFF);
    offset += 4;

    address = PCI_CreateConfigAddress(bus, slot, 0, offset);
    outl(PCI_CONFIG_ADDRESS, address);
    temp = inl(PCI_CONFIG_DATA);
    ret.IOBase = (uint8_t)((temp >> 0) & 0xFF);
    ret.IOLimit = (uint8_t)((temp >> 8) & 0xFF);
    ret.SecondaryStatus = (uint16_t)((temp >> 16) & 0xFFFF);
    offset += 4;

    address = PCI_CreateConfigAddress(bus, slot, 0, offset);
    outl(PCI_CONFIG_ADDRESS, address);
    temp = inl(PCI_CONFIG_DATA);
    ret.MemoryBase = (uint16_t)((temp >> 0) & 0xFFFF);
    ret.MemoryLimit = (uint16_t)((temp >> 16) & 0xFFFF);
    offset += 4;

    address = PCI_CreateConfigAddress(bus, slot, 0, offset);
    outl(PCI_CONFIG_ADDRESS, address);
    temp = inl(PCI_CONFIG_DATA);
    ret.PrefetchMemoryBaseLower = (uint16_t)((temp >> 0) & 0xFFFF);
    ret.PrefetchMemoryLimitLower = (uint16_t)((temp >> 16) & 0xFFFF);
    offset += 4;

    address = PCI_CreateConfigAddress(bus, slot, 0, offset);
    outl(PCI_CONFIG_ADDRESS, address);
    ret.PrefetchBaseUpper = inl(PCI_CONFIG_DATA);
    offset += 4;

    address = PCI_CreateConfigAddress(bus, slot, 0, offset);
    outl(PCI_CONFIG_ADDRESS, address);
    ret.PrefetchLimitUpper = inl(PCI_CONFIG_DATA);
    offset += 4;

    address = PCI_CreateConfigAddress(bus, slot, 0, offset);
    outl(PCI_CONFIG_ADDRESS, address);
    temp = inl(PCI_CONFIG_DATA);
    ret.IOBaseUpper = (uint16_t)((temp >> 0) & 0xFFFF);
    ret.IOLimitUpper = (uint16_t)((temp >> 16) & 0xFFFF);
    offset += 4;

    address = PCI_CreateConfigAddress(bus, slot, 0, offset);
    outl(PCI_CONFIG_ADDRESS, address);
    temp = inl(PCI_CONFIG_DATA);
    ret.CapabilityPointer = (uint16_t)((temp >> 0) & 0xFF);
    offset += 4;

    address = PCI_CreateConfigAddress(bus, slot, 0, offset);
    outl(PCI_CONFIG_ADDRESS, address);
    temp = inl(PCI_CONFIG_DATA);
    ret.ExpansionROM = (uint32_t)((temp >> 0) & 0xFF);
    offset += 4;

    address = PCI_CreateConfigAddress(bus, slot, 0, offset);
    outl(PCI_CONFIG_ADDRESS, address);
    temp = inl(PCI_CONFIG_DATA);
    ret.InterruptLine = (uint8_t)((temp >> 0) & 0xFF);
    ret.InterruptPIN = (uint8_t)((temp >> 8) & 0xFF);
    ret.BridgeControl = (uint16_t)((temp >> 16) & 0xFF);
}

void ScanBusses(){
    uint8_t bus = 0;

    for(uint8_t slot = 0; slot < 32; slot++){
        PCI_Common_Header curHeader = PCI_ReadCommonHeader(bus, slot);
        uint8_t ClassCode = curHeader.ClassCode;
        uint8_t SubClass = curHeader.SubClass;
        uint8_t HeaderType = curHeader.HeaderType;
        uint8_t ProgIF = curHeader.ProgIF;

        if(HeaderType == 0x00){
            PCI_Header_0x0 header0 = PCI_ReadHeader0(curHeader, bus, slot);
        }else if(HeaderType = 0x01){
            PCI_Header_0x1 header1 = PCI_ReadHeader1(curHeader, bus, slot);
        }
    }
}

void PCI_WriteCommand(){

}

void PCI_Init(){

}