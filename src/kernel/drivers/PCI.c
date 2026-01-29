#include "PCI.h"

#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA 0xCFC

#define BUS_COUNT

PCI_Device* AHCI_Controller = NULL;
PCI_Device* IDE_Controller = NULL;

PCI_Device Devices[256];

uint32_t device_count;

uint64_t TwoPieceBARtoPhysAddress(uint32_t BAR0, uint32_t BAR1){
    uint64_t LOWER = ((uint64_t)BAR0); // the lower 32 bits of the address, indicated by BAR0, should be converted to a uint64_t and nothing else shall occur.
    uint64_t UPPER = ((uint64_t)BAR1) << 32; // the upper 32 bits of the address, indicated by BAR1, should be shifted left 32 bits after being converted to a uint64_t to properly convert it.
    return UPPER | LOWER; // Bitwise OR together the UPPER and LOWER variables to finish the addresses.
}

uint32_t PCI_CreateConfigAddress(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset){
    uint32_t lbus = (uint32_t)bus;
    uint32_t lslot = (uint32_t)slot;
    uint32_t lfunc = (uint32_t)func;

    uint32_t address = 
        (uint32_t)
        (
            (lbus << 16) |
            (lslot << 11) |
            (lfunc << 8) |
            (offset & 0xFC) |
            ((uint32_t)0x80000000) // set enable bit
        );
    return address;
}

uint32_t PCI_ReadL(uint8_t bus, uint8_t slot, uint8_t offset){
    uint32_t Address = PCI_CreateConfigAddress(bus, slot, 0, offset);

    outl(PCI_CONFIG_ADDRESS, Address);
    return inl(PCI_CONFIG_DATA);
}

uint16_t PCI_ReadW(uint8_t bus, uint8_t slot, uint8_t offset){
    uint32_t Address = PCI_CreateConfigAddress(bus, slot, 0, offset);

    outl(PCI_CONFIG_ADDRESS, Address);
    uint16_t temp;
    temp = (uint16_t)((inl(PCI_CONFIG_DATA) >> ((offset & 2) * 8)) & 0xFFFF);
    return temp;
}

void PCI_PrintCommonHeader(PCI_Common_Header header){
    char VendorID[10];
    int_to_char_array_hex(header.VendorID, VendorID, sizeof(VendorID), 0);
    char DeviceID[10];
    int_to_char_array_hex(header.DeviceID, DeviceID, sizeof(DeviceID), 0);
    char Command[20];
    int_to_char_array_binary(header.Command, Command, sizeof(Command), 0);
    char Status[20];
    int_to_char_array_binary(header.Status, Status, sizeof(Status), 0);

    char RevisionID[10];
    int_to_char_array_hex(header.RevisionID, RevisionID, sizeof(RevisionID), 0);
    char ProgIF[10];
    int_to_char_array_hex(header.ProgIF, ProgIF, sizeof(ProgIF), 0);
    char SubClass[10];
    int_to_char_array_hex(header.SubClass, SubClass, sizeof(SubClass), 0);
    char ClassCode[10];
    int_to_char_array_hex(header.ClassCode, ClassCode, sizeof(ClassCode), 0);


    char CacheLineSize[10];
    int_to_char_array_hex(header.CacheLineSize, CacheLineSize, sizeof(CacheLineSize), 0);
    char LatencyTimer[10];
    int_to_char_array_hex(header.LatencyTimer, LatencyTimer, sizeof(LatencyTimer), 0);
    char HeaderType[10];
    int_to_char_array_hex(header.HeaderType, HeaderType, sizeof(HeaderType), 0);
    char BIST[10];
    int_to_char_array_hex(header.BIST, BIST, sizeof(BIST), 0);

    printf("VendorID:        ",0); printf(VendorID, 0); printf("\n", 0);
    printf("DeviceID:        ",0); printf(DeviceID, 0); printf("\n", 0);
    printf("Command:         ",0); printf(Command, 0); printf("\n", 0);
    printf("Status:          ",0); printf(Status, 0); printf("\n", 0);

    printf("RevisionID:      ",0); printf(RevisionID, 0); printf("\n", 0);
    printf("ProgIF:          ",0); printf(ProgIF, 0); printf("\n", 0);
    printf("SubClass:        ",0); printf(SubClass, 0); printf("\n", 0);
    printf("ClassCode:       ",0); printf(ClassCode, 0); printf("\n", 0);
    
    printf("CacheLineSize:   ",0); printf(CacheLineSize, 0); printf("\n", 0);
    printf("LatencyTimer:    ",0); printf(LatencyTimer, 0); printf("\n", 0);
    printf("HeaderType:      ",0); printf(HeaderType, 0); printf("\n", 0);
    printf("BIST:            ",0); printf(BIST, 0); printf("\n", 0);
}

PCI_Common_Header PCI_ReadCommonHeader(uint8_t bus, uint8_t slot){
    PCI_Common_Header ret;
    uint32_t* retP = (uint32_t*)(&ret);
    /*
        hope this works, basically treats the common header
        as memory, then does a memcopy type thing to paste data
        straight into the struct without using real struct access
        more efficient and hopefully optimized.
    */
    for(int i = 0; i < 4; i++){
        uint32_t temp = PCI_ReadL(bus, slot, i*4);
        retP[i] = temp;
    }

    return ret;
}

PCI_Header_0x0 PCI_ReadHeader0(PCI_Common_Header common, uint8_t bus, uint8_t slot){
    PCI_Header_0x0 ret;
    uint32_t* retP = (uint32_t*)(&ret);

    ret.Common_Header = common;

    for(int i = 0; i < 16; i++){
        uint32_t temp = PCI_ReadL(bus, slot, (i+4)*4);
        retP[i] = temp;
    }

    return ret;
}

PCI_Header_0x1 PCI_ReadHeader1(PCI_Common_Header common, uint8_t bus, uint8_t slot){
    PCI_Header_0x1 ret;
    uint32_t* retP = (uint32_t*)(&ret);

    ret.Common_Header = common;

    for(int i = 0; i < 16; i++){
        uint32_t temp = PCI_ReadL(bus, slot, (i+4)*4);
        retP[i] = temp;
    }

    return ret;
}

void ScanBusses(){
    #ifdef DEBUG
        printf_debug("\nVendID DevcID\tCLASS   SUBCLASS   ProgIF\n", 0);
    #endif

    uint8_t ClassCode;
    uint8_t SubClass;
    uint8_t HeaderType;
    uint8_t ProgIF;
    bool header_type;
    int deviceRun = 0;

    // loop through every bus and every slot of said bus, then add whichever device we've scanned to devices.
    // only scans 8 busses
    for(uint8_t bus = 0; bus < 8; bus++){
        for(uint8_t slot = 0; slot < 32; slot++){
            PCI_Device Cur_Device;
            PCI_Header_0x0 header0;
            PCI_Header_0x1 header1;
            PCI_Common_Header curHeader = PCI_ReadCommonHeader(bus, slot);
            ClassCode = curHeader.ClassCode;
            SubClass = curHeader.SubClass;
            HeaderType = curHeader.HeaderType;
            ProgIF = curHeader.ProgIF;

            if(curHeader.VendorID == 0xFFFF){
                continue;
            }

            if(HeaderType == 0x00){
                header0 = PCI_ReadHeader0(curHeader, bus, slot);
                header_type = false;
            }else if(HeaderType = 0x01){
                header1 = PCI_ReadHeader1(curHeader, bus, slot);
                header_type = true;
            }

            Cur_Device.Header = curHeader;
            Cur_Device.Header0 = header0;
            Cur_Device.Header1 = header1;
            Cur_Device.header_type = header_type;
            Cur_Device.present = true;
            Cur_Device.bus = bus;
            Cur_Device.slot = slot;

            Devices[deviceRun] = Cur_Device;

            #ifdef DEBUG
                char test_char[22];
                int_to_char_array_hex(curHeader.VendorID, test_char, sizeof(test_char), 4);
                printf_debug(test_char, 0);
                printf(":", 0);
                int_to_char_array_hex(curHeader.DeviceID, test_char, sizeof(test_char), 4);
                printf_debug(test_char, 0);
                printf("\t", 0);

                int_to_char_array_hex(curHeader.ClassCode, test_char, sizeof(test_char), 5);
                printf_debug(test_char, 0);
                printf(":", 0);
                int_to_char_array_hex(curHeader.SubClass, test_char, sizeof(test_char), 8);
                printf_debug(test_char, 0);
                printf(":", 0);
                int_to_char_array_hex(curHeader.ProgIF, test_char, sizeof(test_char), 4);
                printf_debug(test_char, 0);
                printf("\n", 0);
            #endif

            if(curHeader.ClassCode == PCI_CLASS_STR && curHeader.SubClass == 0x06 && curHeader.ProgIF == 0x01){
                AHCI_Controller = &Devices[deviceRun];
            }else if(curHeader.ClassCode == PCI_CLASS_STR && curHeader.SubClass == 0x01){
                IDE_Controller = &Devices[deviceRun];
                printf("ide\n", 0);
            }
            deviceRun++;
        }
    }
    device_count = deviceRun;

    #ifdef DEBUG
        char test_char[22];
        printf_debug("Detected ", 0);
        int_to_char_array(device_count, test_char, sizeof(test_char), 0);
        printf_debug(test_char, 0);
        printf_debug(" devices.\n", 0);
    #endif
} 

void pci_writeb(uint32_t address, uint8_t data){
    outl(PCI_CONFIG_ADDRESS, address);
    outb(PCI_CONFIG_DATA, data);
}

void pci_writew(uint32_t address, uint16_t data){
    outl(PCI_CONFIG_ADDRESS, address);
    outw(PCI_CONFIG_DATA, data);
}

void PCI_Update_Important_Info(PCI_Device* device){
    uint8_t bus = device->bus;
    uint8_t slot = device-> slot;
    
    uint32_t COMBO = PCI_ReadL(bus, slot, 0x4);

    uint16_t COMMAND = (uint16_t)(COMBO & 0xFFF);
    uint16_t STATUS = (uint16_t)((COMBO >> 16) & 0xFFF);

    device->Header.Command = COMMAND;
    device->Header.Status = STATUS;
}

void PCI_Write_Command_Register(PCI_Device* device, PCI_Command_Register command){
    uint8_t bus = device->bus;
    uint8_t slot = device->slot;
    
    uint32_t address = PCI_CreateConfigAddress(bus, slot, 0, 0x4);

    uint16_t New_Command = (
        command.IO_Space |
        (command.Memory_Space << 1) |
        (command.Bus_Master << 2) |
        (command.Parity_ERR_Response << 6) |
        (command.SERR_Enable << 8) |
        (command.Interrupt_Disable << 10)
    );

    pci_writew(address, New_Command);

    PCI_Update_Important_Info(device);
}

void PCI_Write_Status_Register(PCI_Device* device, PCI_Command_Register command){
    uint8_t bus = device->bus;
    uint8_t slot = device->slot;
    
    uint32_t address = PCI_CreateConfigAddress(bus, slot, 0, 0x6);

    uint16_t New_Command = (
        command.IO_Space |
        (command.Memory_Space << 1) |
        (command.Bus_Master << 2) |
        (command.Parity_ERR_Response << 6) |
        (command.SERR_Enable << 8) |
        (command.Interrupt_Disable << 10)
    );

    pci_writew(address, New_Command);

    PCI_Update_Important_Info(device);
}