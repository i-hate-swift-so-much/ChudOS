#include "Devices/Disk/Floppy.h"

#define FLOPPY_IRQ_INDEX 0x26
#define FLOPPY_INFO_STRUCT_ADDRESS 0x1FFA // this is the address for info about the floppy which is written to by boot1.s

FLOPPY_Drive_Info FLOPPY_Drives[4];
bool FLOPPY_FDC_Present = false;

// makes sure the motherboard has an actual FDC
void FLOPPY_Check_FDC(){
    enum FLOPPY_Controller_Registers MSR = MAIN_STATUS_REGISTER;
    FLOPPY_FDC_Present = inb(MSR) != 0;
}

CHS FLOPPY_Get_CHS_Count(uint8_t Drive){

}

bool FLOPPY_Can_Send_FIFO(){
    enum FLOPPY_Controller_Registers MSR = MAIN_STATUS_REGISTER;
    return (inb(MSR) >> 7) & 1;
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

void FLOPPY_Configure(uint8_t Drive){
    FLOPPY_Set_Drive(Drive);

    enum FLOPPY_Controller_Registers MSR = MAIN_STATUS_REGISTER;
    
    bool Available = FLOPPY_Can_Send_FIFO();
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
        printf_debug("\nRegistered IRQ\t", 0);
    #endif

    // second, set up the IRQ
    SetIDTEntry(FLOPPY_IRQ_INDEX, (uint64_t)floppy_drive_stub, 0x08, 0x8E, 0x01);

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
    return (state & motor_bit) == 1;
}

CHS FLOPPY_LBA_To_CHS(uint64_t LBA){

}