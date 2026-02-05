#include "Devices/Disk/Floppy.h"

#define FLOPPY_IRQ_INDEX 0x26
#define FLOPPY_INFO_STRUCT_ADDRESS 0x7DB8 // this is the address for info about the floppy which is written to by boot0.s

FLOPPY_Drive_Info FLOPPY_Drives[4];
bool FLOPPY_FDC_Present = false;

FLOPPY_Info_Struct FLOPPY_Main_Info;

// makes sure the motherboard has an actual FDC
void FLOPPY_Check_FDC(){
    enum FLOPPY_Controller_Registers MSR = MAIN_STATUS_REGISTER;
    FLOPPY_FDC_Present = inb(MSR) != 0;
}

void FLOPPY_Send_FIFO(uint8_t Byte){
    enum FLOPPY_Controller_Registers FIFO = DATA_FIFO;

    outb(FIFO, Byte);
}

/*
    Sends a command byte then attribute byte to the FIFO
    Return codes:
        0x00 : Success
        0x01 : Improper DIO
*/
int FLOPPY_Send_Command(uint8_t Command_Byte, uint8_t Parameter_Count, uint8_t* Parameter_Bytes){
    enum FLOPPY_Controller_Registers FIFO = DATA_FIFO;
    enum FLOPPY_Controller_Registers MSR = MAIN_STATUS_REGISTER;

    // Send command byte
    FLOPPY_Send_FIFO(Command_Byte);

    for(int i = 0; i < Parameter_Count; i++){
        uint64_t Timestamp = TimerWindow;
        
        // wait for MSR.RQM to be 1
        while(inb(MSR) & 0x80 != 0x80 && Timestamp-TimerWindow < 1000){}

        // make sure DIO is 0
        bool DIO = inb(MSR) & 0x40;
        if(DIO != false){ return 0x01; }

        // send parameter byte
        FLOPPY_Send_FIFO(Parameter_Bytes[i]);
    }
}

bool FLOPPY_Can_Send_FIFO(uint8_t Drive){
    enum FLOPPY_Controller_Registers MSR = MAIN_STATUS_REGISTER;
    uint8_t MSR_State = inb(MSR);
    return ((MSR_State >> 7) & 1) && !((MSR_State >> Drive) & 1);
}

void FLOPPY_Set_Drive(uint8_t Drive){
    // set drive number
    enum FLOPPY_Controller_Registers DOR = DIGITAL_OUTPUT_REGISTER;
    uint8_t DOR_Status = inb(DOR);
    
    // clear the drive number
    DOR_Status &= ~(3); // 0b11111100

    DOR_Status |= Drive & 3; // first 2 bits. indexes: 0-3
    outb(DOR, DOR_Status);
}

void FLOPPY_Recalibrate(uint8_t Drive){
    FLOPPY_Set_Drive(Drive);
}

/*
    Sets the appropriate bits for sending a command
*/
void FLOPPY_CMD_OUT_Prepare(){
    enum FLOPPY_Controller_Registers MSR = MAIN_STATUS_REGISTER;

    uint8_t RQM = 1 << 7; // RQM bit
    uint8_t DIO = 1 << 6; // DIO bit (this bit should be cleared)

    uint8_t MSR_Status = inb(MSR);
    MSR_Status |= RQM;
    MSR_Status &= ~DIO; // this clears the DIO bit
}

void FLOPPY_Configure(uint8_t Drive){
    FLOPPY_Set_Drive(Drive);

    enum FLOPPY_Controller_Registers MSR = MAIN_STATUS_REGISTER;
    
    bool Available = FLOPPY_Can_Send_FIFO(Drive);
}

/*
    Sets up the IRQs for the controller and enables it. Also resets the controller.
*/
int FLOPPY_Init_Controller(){
    // first, reset the controller
    enum FLOPPY_Controller_Registers DOR = DIGITAL_OUTPUT_REGISTER;
    uint8_t DOR_Status = inb(DOR);
    uint8_t reset_status = (DOR_Status >> 2) & 1;

    if(reset_status == 0){
        DOR_Status &= ~(1 << 2);
        outb(DOR, DOR_Status);

        uint64_t timestamp = SecondsSinceBoot;
        while(SecondsSinceBoot-timestamp < 1){}
        // set the bit back, since it's probably been reset
        DOR_Status = inb(DOR);
        DOR_Status |= (1 << 2);
        outb(DOR, DOR_Status);
    }  

    #ifdef DEBUG
        print_debug("\nReset FDC\n", 0);
        print_debug("Registered IRQ\t", 0);
    #endif

    // second, set up the IRQ
    SetIDTEntry(FLOPPY_IRQ_INDEX, (uint64_t)floppy_drive_stub, 0x08, 0x8E, 0x01);

    // third, enable the interrupt line

    DOR_Status = inb(DOR);
    DOR_Status |= (1 << 2);
    outb(DOR, DOR_Status);
    DOR_Status = inb(DOR);
    #ifdef DEBUG
        if(DOR_Status & (1 << 2)){
            print_debug("Enabled IRQ line\n", 0);
        }else{
            print_error_snapshot("Couldn't Enable IRQ Line", 0);
            return 0;
        }
    #endif

    print_success_snapshot("SUCCESS", 0);

    return 1; // success
}

void FLOPPY_IRQ(InterruptRegisters* registers){

}

/*
    Turns on the motor for a drive
*/
int FLOPPY_Init_Drive(uint8_t drive_number){
    drive_number %= 4;
    uint8_t motor_bit = (1 << (4 + drive_number));
    
    // turn on the motor for the specified drive
    enum FLOPPY_Controller_Registers DOR = DIGITAL_OUTPUT_REGISTER;
    uint8_t state = inb(DOR);
    state |= motor_bit;
    outb(DOR, state);

    // check if the motor is on
    state = inb(DOR);

    if(!(state & motor_bit)){
        print_error_snapshot("Couldn't enable Motor\n", 0);
        return 0;
    }

    #ifdef DEBUG
        print_debug("\nEnabled Motor\n", 0);
    #endif

    memcpy((void*)&FLOPPY_Main_Info, (void*)FLOPPY_INFO_STRUCT_ADDRESS, 6);

    #ifdef DEBUG
        print_debug("Set max CHS\n", 0);
        SetTextColor(LCYAN, BLACK);
        printf("%i:%i:%i\n", FLOPPY_Main_Info.cylinder_max, FLOPPY_Main_Info.head_max, FLOPPY_Main_Info.sector_max);
        SetTextColor(WHITE, BLACK);
    #endif

    print_success_snapshot("SUCCESS", 0);

    return (state & motor_bit);
}

CHS FLOPPY_LBA_To_CHS(uint64_t LBA){
    
}