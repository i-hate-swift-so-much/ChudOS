#include "Devices/Disk/AHCI.h"
#include "Devices/PS2/Keyboard.h"

AHCI_MMIO* AHCI_Main_MMIO;

bool AHCI_Ownership = false;
bool AHCI_Reset = false;

bool AHCI_S64A = false;
bool AHCI_Command_Queuing = false;

int AHCI_IRQ_LINE;
/*
    Performs the OS/BIOS handoff

    note: inefficient , should probably find some way to avoid `while(BIOS_BUSY){}`
*/
void AHCI_BIOS_Handoff(){
    AHCI_Main_MMIO->Handoff_Status |= 0b100; // set SMI
    AHCI_Main_MMIO->Handoff_Status |= 0b10; // set OOS
    while(AHCI_Main_MMIO->Handoff_Status & 1 || AHCI_Main_MMIO->Handoff_Status & 0b10000){}
    AHCI_Ownership = ~(AHCI_Main_MMIO->Handoff_Status) & 1;
}

/*
    Performs an HBA reset as defined in AHCI Specification 10.4.3

    Suffers from the same issue as AHCI_BIOS_Handoff
    note: if hardware does not reset GHC.HR to 0 after 1 second, then controller is hung or non-functional.
*/
void AHCI_HBA_Reset(){
    uint32_t* GHC = &AHCI_Main_MMIO->Global_Host_Control;

    *GHC |= 1; // set bit GHC.HR, tell AHCI to initialize a HardwareReset
    uint64_t start_window = TimerWindow;
    // wait until bit is cleared or time is past 1 second
    while(*GHC & 1 && (start_window-TimerWindow)/Frequency < 1){

    }
    if((start_window-TimerWindow)/Frequency > 1) { 
        print_error_snapshot("FAIL: Couldn't reset HBA\n", 0);

        printf("Press ENTER to retry, or any other key to quit boot.\n");
        should_not_proceed = false;
        should_proceed = false;
        // poll until the user decides
        while(should_not_proceed == false && should_proceed == false){}
        if(should_not_proceed){
            printf("QUIT\n");
        }else{
            AHCI_HBA_Reset();
            return;
        }

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
        malloc_specific(KernelTask, (uint64_t)curPort, &flags);
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

    AHCI_Main_MMIO = (AHCI_MMIO*)malloc_specific(KernelTask, device->Header0.BAR5, &flags);
    (AHCI_MMIO*)malloc_specific(KernelTask, device->Header0.BAR5+0x1000, &flags);
    #ifdef DEBUG
        print_debug("\nSuccessfully setup HBA MMIO\n", 0);
    #endif

    // if we should perform a handoff, then do that
    if(AHCI_Main_MMIO->Extended_Host_Capabilities & 1 == 0)
    { 
        AHCI_BIOS_Handoff(); 
        if(AHCI_Ownership == false){
            print_error_snapshot("FAIL: Couldn't Obtain Ownership \n", 0);

            #ifdef DEBUG
            char test_char[22];
            int_to_char_array_binary(AHCI_Main_MMIO->Handoff_Status, test_char, sizeof(test_char), 0);
            print_debug(test_char, 0);
            print("\n", 0);
            #endif

            return;
        }
    }else{
        #ifdef DEBUG
            print_debug("No need for ownership detected.\n", 0);
        #endif
    }
    if(AHCI_Main_MMIO == NULL)
    { 
        print_error_snapshot("FAIL: AHCI_Main_MMIO is null. \n", 0);

        return;
    }    

    //HBA reset (AHCI 10.4.3)
    //just set GHC.HR to 1,
    //when GHC.HR is cleared, 
    //it's been reset properly
    AHCI_HBA_Reset();

    #ifdef DEBUG
        print_debug("Succesfully reset HBA\n", 0);
    #endif

    SetIDTEntry(AHCI_IRQ_LINE+0x20, (uint64_t)ahci_interrupt_stub, 0x08, 0x8E, 0x00);
    pic_unmask(AHCI_IRQ_LINE);

    #ifdef DEBUG
        print_debug("Succesfully registered AHCI IDT entry\n", 0);
    #endif

    // AHCI specification 3.1.2
    AHCI_Main_MMIO->Global_Host_Control |= (1 << 31); // this sets the AHCI enable
    AHCI_Main_MMIO->Global_Host_Control |= (1 << 1); // this sets the interrupt enable

    #ifdef DEBUG
        print_debug("Succesfully enabled AHCI mode and interrupts\n", 0);
    #endif

    AHCI_S64A = ((AHCI_Main_MMIO->Host_Capabilities) >> 31) & 1;
    AHCI_Command_Queuing = ((AHCI_Main_MMIO->Host_Capabilities) >> 30) & 1;

    // set up ports and their command tables
    AHCI_Setup_Port(0);

    print_success_snapshot("SUCCESS                    ", 0);
}

void HandleAHCIInterrupt(InterruptRegisters* registers){
    printf("AHCI\n");
    pic_send_eoi(AHCI_IRQ_LINE);
}