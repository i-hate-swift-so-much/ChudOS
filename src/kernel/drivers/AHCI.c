#include "AHCI.h"

AHCI_MMIO* AHCI_Main_MMIO;

bool AHCI_Ownership = false;
bool AHCI_Reset = false;

/*
    Performs the OS/BIOS handoff

    note: inefficient , should probably find some way to avoid `while(BIOS_BUSY){}`
*/
void AHCI_BIOS_Handoff(){
    uint8_t* BUSY = (uint8_t*)(AHCI_Main_MMIO->Handoff_Status);
    uint32_t start_handoff = 0b110; // request a transfer. SMI bit must be set
    AHCI_Main_MMIO->Handoff_Status |= start_handoff;
    while(*BUSY & 0b11 != 0b10){}
    AHCI_Ownership = *BUSY >> 1;
}

/*
    Performs an HBA reset as defined in AHCI Specification 10.4.3

    Suffers from the same issue as AHCI_BIOS_Handoff
    note: if hardware does not reset GHC.HR to 0 after 1 second, then controller is hung or non-functional.
*/
uint8_t HBA_Reset_Count = 0;
void AHCI_HBA_Reset(){
    uint32_t* GHC = AHCI_Main_MMIO->Global_Host_Control;

    *GHC |= 1; // set bit GHC.HR, tell AHCI to initialize a HardwareReset
    uint64_t start_window = TimerWindow;
    // wait until bit is cleared or time is past 1 second
    while(*GHC & 1 && (start_window-TimerWindow)/Frequency < 1){

    }
    if((start_window-TimerWindow)/Frequency > 1) { 
        return;
    }
    AHCI_Reset = true;
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

    //HBA reset (AHCI 10.4.3)
    //just set GHC.HR to 1,
    //when GHC.HR is cleared, 
    //it's been reset properly
    AHCI_HBA_Reset();    

    AHCI_Main_MMIO->Global_Host_Control |= (1 << 31); // this sets the AHCI enable

    
}