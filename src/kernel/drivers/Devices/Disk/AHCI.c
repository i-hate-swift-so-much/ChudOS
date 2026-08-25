#include "Devices/Disk/AHCI.h"
#include "Devices/PS2/Keyboard.h"

volatile struct AHCI_MMIO* AHCI_Main_MMIO;

bool AHCI_Ownership = false;
bool AHCI_Reset = false;

bool AHCI_S64A = false;
bool AHCI_Command_Queuing = false;

int AHCI_IRQ_LINE;
/*
    Performs the OS/BIOS handoff

    note: inefficient, should probably find some way to avoid `while(BIOS_BUSY){}`
*/
void AHCI_BIOS_Handoff(){
    AHCI_Main_MMIO->BOHC.SOOE = true; // set SMI
    AHCI_Main_MMIO->BOHC.OOS = true; // set OOS
    while(AHCI_Main_MMIO->BOHC.BB == true && AHCI_Main_MMIO->BOHC.BOS){}
    AHCI_Ownership = AHCI_Main_MMIO->BOHC.OOS && AHCI_Main_MMIO->BOHC.BOS == false;
}

/*
    Performs an HBA reset as defined in AHCI Specification 10.4.3

    Suffers from the same issue as AHCI_BIOS_Handoff
    note: if hardware does not reset GHC.HR to 0 after 1 second, then controller is hung or non-functional.
*/
void AHCI_HBA_Reset(){
    volatile uint32_t* GHC = &AHCI_Main_MMIO->Global_Host_Control;

    *GHC |= 1; // set bit GHC.HR, tell AHCI to initialize a HardwareReset
    uint64_t start_window = TimerWindow;
    // wait until bit is cleared or time is past 1 second
    while(*GHC & 1 && (start_window-TimerWindow)/Frequency < 1){}
    if((start_window-TimerWindow)/Frequency > 1) { 
        virtualprint(KERNEL_T, "[ahci] fail no reset\n");
        AHCI_Reset = false;
        return;
    }
    AHCI_Reset = true;
}

void AHCI_Setup_Port(uint8_t index){
    AHCI_PORT* curPort = &(AHCI_Main_MMIO->Ports[index]);

    PagePermissions flags;
    flags.flags = KERNEL_FLAGS_UNCACHEABLE;
    flags.Execute_Disable = false;

    if(index % 4 == 0){
        malloc_specific(KernelTask, (uint64_t)curPort-VIRTUAL_MEMORY_BARRIER, &flags);
    }

    uint64_t nextFree = FindNextFreePhysical();

    curPort->Command_List_BAR = (uint64_t)malloc_specific(KernelTask, nextFree, &flags);
}

/*
    Used to initialize an AHCI device, automatically initializes MMIO, fills command register, etc.
    Checklist:
        1. Enable interrupts, DMA, and memory space in Command Register
        2. Map BAR 5 to memory
        3. Perform BIOS/OS handoff if needd
*/
void AHCI_Init(PCI_Device* device){
    AHCI_IRQ_LINE = device->Header0.InterruptLine;
    
    PCI_Command_Register init_cmd;
    init_cmd.Memory_Space = true;
    init_cmd.Interrupt_Disable = false;
    init_cmd.Bus_Master = true;
    
    PagePermissions flags;
    flags.flags = KERNEL_FLAGS_UNCACHEABLE;
    flags.Execute_Disable = false;

    AHCI_Main_MMIO = (struct AHCI_MMIO*)malloc_specific(KernelTask, device->Header0.BAR5, &flags);
    (struct AHCI_MMIO*)malloc_specific(KernelTask, device->Header0.BAR5+0x1000, &flags);
    virtualprint(KERNEL_T, "[ahci] MMIO setup\n");

    // if we should perform a handoff, then do that
    if(AHCI_Main_MMIO->Extended_Host_Capabilities & 1 == 0)
    { 
        AHCI_BIOS_Handoff(); 
        if(AHCI_Ownership == false){
            virtualprint(KERNEL_T, "[ahci] fail no ownership\n");

            #ifdef DEBUG
            char test_char[22];
            int_to_char_array_binary(AHCI_Main_MMIO->Handoff_Status, test_char, sizeof(test_char), 0);
            print_debug(test_char, 0);
            print("\n", 0);
            #endif

            return;
        }
        virtualprint(KERNEL_T, "[ahci] obtained ownership\n");
    }else{
        virtualprint(KERNEL_T, "[ahci] no switch\n");
    }
    if(AHCI_Main_MMIO == NULL)
    { 
        virtualprint(KERNEL_T, "[ahci] fail bad MMIO\n");

        return;
    }    

    //HBA reset (AHCI 10.4.3)
    //just set GHC.HR to 1,
    //when GHC.HR is cleared, 
    //it's been reset properly
    AHCI_HBA_Reset();

    virtualprint(KERNEL_T, "[ahci] reset true\n");

    SetIDTEntry(AHCI_IRQ_LINE+0x20, (uint64_t)ahci_interrupt_stub, 0x08, 0x8E, 0x00);
    pic_unmask(AHCI_IRQ_LINE);

    // AHCI specification 3.1.2
    AHCI_Main_MMIO->Global_Host_Control |= (1 << 31); // this sets the AHCI enable
    AHCI_Main_MMIO->Global_Host_Control |= (1 << 1); // this sets the interrupt enable

    AHCI_S64A = ((AHCI_Main_MMIO->Host_Capabilities) >> 31) & 1;
    AHCI_Command_Queuing = ((AHCI_Main_MMIO->Host_Capabilities) >> 30) & 1;

    // set up ports and their command tables
    AHCI_Setup_Port(0);

    virtualprint(KERNEL_T, "[ahci] success\n");
}

void HandleAHCIInterrupt(InterruptRegisters* registers){
    virtualprint(KERNEL_T, "[ahci.int] ignored\n");
    pic_send_eoi(AHCI_IRQ_LINE);
}