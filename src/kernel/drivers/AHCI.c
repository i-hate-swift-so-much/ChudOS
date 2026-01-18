#include "AHCI.h"

AHCI_MMIO* AHCI_Main_MMIO;

bool AHCI_Ownership;

/*
    Performs the OS/BIOS handoff

    note: insanely slow, should probably find some way to avoid `while(BIOS_BUSY){}`
*/
void AHCI_BIOS_Handoff(){
    uint8_t* BUSY = (uint8_t*)(AHCI_Main_MMIO->Handoff_Status);
    uint32_t start_handoff = 0b110; // request a transfer. SMI bit must be set
    AHCI_Main_MMIO->Handoff_Status |= start_handoff;
    while(*BUSY & 0b11 != 0b10){}
    AHCI_Ownership = *BUSY >> 1;
}

/*
    Used to initialize an AHCI device, automatically initializes MMIO, fills command register, etc.
    Checklist:
        1. Enable interrupts, DMA, and memory space in Command Register
        2. Map BAR 5 to memory
        3. Perform BIOS/OS handoff
*/
void AHCI_Init(PCI_Device* device){
    //return;
    PCI_Command_Register init_cmd;
    init_cmd.Memory_Space = true;
    init_cmd.Interrupt_Disable = false;
    init_cmd.Bus_Master = true;
    
    PagePermissions flags;
    flags.flags = KERNEL_FLAGS_UNCACHEABLE;
    flags.Execute_Disable = false;

    AHCI_Main_MMIO = (AHCI_MMIO*)malloc_specific(&KernelTask, device->Header0.BAR5, &flags);

    //mem_bitmap_dump(16);
    if(AHCI_Main_MMIO != NULL)
    { AHCI_BIOS_Handoff(); }
    else
    { 
        SetTextColor(LRED, BLACK);
        printf("FAIL: AHCI_Main_MMIO is null. \n", 0);
        SetTextColor(WHITE, BLACK);
    }    

    AHCI_Main_MMIO->Global_Host_Control |= (1 << 31); // this sets the AHCI enable
}