#include "Devices/Disk/Floppy.h"

//https://wiki.osdev.org/Floppy_Disk_Controller


#define FLOPPY_IRQ_INDEX 0x26
#define FLOPPY_INFO_STRUCT_ADDRESS 0x7DB8 + 0xFFFF800000000000 // this is the address for info about the floppy which is written to by boot0.s

// a 64 KiB bounce back buffer aligned to 64 KiB
uint8_t FLOPPY_BOUNCE_BUFFER[0x10000]__attribute__((aligned(0x10000)))__attribute__((section(".under_16")));

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
    Checks the BUSY and DIO bit of the MSR to make sure the FDC is properly terminated
*/
bool FLOPPY_Terminated(){ 
    enum FLOPPY_Controller_Registers MSR = MAIN_STATUS_REGISTER;
    uint8_t MSR_Status = inb(MSR);
    // BUSY == 0 && DIO == 0
    return ((MSR_Status >> 4) & 1) && ((~MSR_Status >> 6) & 1);
}

/*
    Reads amount result bytes into buffer from FIFO. Checks the DIO and RQM bit
*/
bool FLOPPY_Read_Result_Bytes(uint8_t count, uint8_t* buffer){
    enum FLOPPY_Controller_Registers MSR = MAIN_STATUS_REGISTER;
    enum FLOPPY_Controller_Registers FIFO = DATA_FIFO;

    uint8_t MSR_Status = 0;
    
    for(int i = 0; i < count; i++){
        // check DIO and RQM
        MSR_Status = inb(MSR);
        if( ((MSR_Status >> 7) & 1) && ((MSR_Status >> 6) & 1) ){
            buffer[i] = inb(FIFO);
        }else{ return false; }
    }
    return true;
}

/*
    Sends a command byte then attribute byte to the FIFO
*/
void FLOPPY_Send_Command(uint8_t Command_Byte, uint8_t Parameter_Count, uint8_t* Parameter_Bytes){
    enum FLOPPY_Controller_Registers FIFO = DATA_FIFO;
    enum FLOPPY_Controller_Registers MSR = MAIN_STATUS_REGISTER;

    // Send command byte
    FLOPPY_Send_FIFO(Command_Byte);

    if(Parameter_Bytes == NULL){ return; }
    for(int i = 0; i < Parameter_Count; i++){
        uint64_t Timestamp = TimerWindow;
        
        // wait for MSR.RQM to be 1 and MSR.DIO to be 0
        while(inb(MSR) & 0x80 != 0x80 || inb(MSR) & 0x40 == 0x40 && Timestamp-TimerWindow < 1000){}

        // send parameter byte
        FLOPPY_Send_FIFO(Parameter_Bytes[i]);
    }
}

bool FLOPPY_Can_Send_FIFO(uint8_t Drive){
    enum FLOPPY_Controller_Registers MSR = MAIN_STATUS_REGISTER;
    uint8_t MSR_State = inb(MSR);
    // RQM == 0 && DOR == 0 && drive not seeking
    return ((MSR_State >> 7) & 1) && ((~MSR_State >> Drive) & 1) && ((~MSR_State >> 6) & 1);
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

/*
    Sets up the IRQs for the controller and enables it. Also resets the controller.
*/
int FLOPPY_Init_Controller(){
    // first, reset the controller
    enum FLOPPY_Controller_Registers DOR = DIGITAL_OUTPUT_REGISTER;
    enum FLOPPY_Controller_Registers MSR = MAIN_STATUS_REGISTER;
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
        print_debug("Registering IRQ\t", 0);
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

        print_debug("REGISTER DUMP\n", 0);
        SetTextColor(LCYAN, BLACK);
        uint8_t MSR_Status = inb(MSR);
        DOR_Status = inb(DOR);
        printf("MAIN STATUS REGISTER: %b\n", MSR_Status);
        printf("DIGITAL OUTPUT REGISTER: %b\n", DOR_Status);

        uint8_t FLOPPY_TEST = FLOPPY_Get_Version(0);
        printf("FLOPPY TEST: %x EXPECTED: 0x90\n", FLOPPY_TEST);
        SetTextColor(WHITE, BLACK);

    #endif

    print_success_snapshot("SUCCESS", 0);

    return 1; // success
}

void FLOPPY_IRQ(InterruptRegisters* registers){
    printf("YOOOO FLOPPY INTERRUPT IN THE HOUSEEE\n", 0);
}

/*
    Turns on the motor for a drive
*/
int FLOPPY_Init_Drive(uint8_t drive_number){
    enum FLOPPY_Controller_Registers MSR = MAIN_STATUS_REGISTER;
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
        
        print_debug("REGISTER DUMP\n", 0);
        SetTextColor(LCYAN, BLACK);
        uint8_t MSR_Status = inb(MSR);
        uint8_t DOR_Status = inb(DOR);
        printf("MAIN STATUS REGISTER: %b\n", MSR_Status);
        printf("DIGITAL OUTPUT REGISTER: %b\n", DOR_Status);
        SetTextColor(WHITE, BLACK);
    #endif

    print_success_snapshot("SUCCESS", 0);

    return (state & motor_bit);
}

CHS FLOPPY_LBA_To_CHS(uint64_t LBA){
    CHS toRet;
    toRet.Cylinder = LBA / (FLOPPY_Main_Info.head_count * FLOPPY_Main_Info.sector_max);
    toRet.Head = (LBA / FLOPPY_Main_Info.sector_max) % FLOPPY_Main_Info.head_count;
    toRet.Sector = (LBA % FLOPPY_Main_Info.sector_max) + 1;
    return toRet;
}

void FLOPPY_Configure(uint8_t Drive, bool Use_Implied_Seek, bool Disable_FIFO, bool Disable_PIO){
    FLOPPY_Set_Drive(Drive);
    
    uint8_t to_send = (Use_Implied_Seek << 6) | (Disable_FIFO << 5) | (Disable_PIO << 4) | 7;

    // threshold is 8, even though we don't use PIO
    uint8_t Parameters[3] = {0, to_send, 0};

    FLOPPY_Send_Command(FLOPPY_CMD_CONFIGURE, 3, Parameters);
} 

/*
    Initiates the VERSION command and returns the result.
    0x90 = the standard intel fdc
*/
uint8_t FLOPPY_Get_Version(uint8_t Drive){
    FLOPPY_Set_Drive(Drive);

    uint8_t Parameters[1];
    
    uint8_t Result_Bytes[1];

    FLOPPY_Send_Command(FLOPPY_CMD_VERSION, 0, Parameters);
    FLOPPY_Read_Result_Bytes(1, Result_Bytes);
    return Result_Bytes[0];
}

/**
    * @brief Reads sectors from a FLOPPY by reprogramming the ISA-DMA and then issuing a READ command with it's parameters.
    This function uses a bounce buffer (at the top of this file) thats read into then memcpy'd into the real buffers
    location.
    * @param Drive The drive index
    * @param Cylinder The starting Cylinder to read from
    * @param Head The starting Head to read from
    * @param Sector The starting Sector to read from
    * @param BufferAddress the address of the buffer to write to
    * @param SectorCount The amount of sectors to read
    * @return int 0 on success, anything else is the error code
*/
int FLOPPY_Read_CHS(uint8_t Drive, uint8_t Cylinder, uint8_t Head, uint8_t Sector, uint64_t BufferAddress, uint16_t SectorCount){
    uint64_t transfer_count = SectorCount * 512; // convert the sector count to a byte count for DMA
    
    FLOPPY_Set_Drive(Drive);

    uint8_t Parameters[8] = {(Head << 2) | Drive, Cylinder, Head, Sector, 2, FLOPPY_Main_Info.sector_max, 0x1b, 0xff};
    uint8_t Results[8];

    // if it will cause the 64kb overflow
    if(transfer_count > 0x10000){
        for(int i = 0; i < (transfer_count / 0x10000); i++){
            // program ISA-DMA
            ISA_DMA_Transfer_To(2, (uint32_t)FLOPPY_BOUNCE_BUFFER, (transfer_count % 0x10000) - 1, true, 0b01);

            // send the command
            FLOPPY_Send_Command(FLOPPY_CMD_READ, 8, Parameters);

            FLOPPY_Read_Result_Bytes(7, Results);

            // do error checking from st1 (Results[1])
            if(Results[1] & 0x80){
                print_error("FLOPPY READ ERROR: NOT ENOUGH SECTORS (0x80)\n", 0);
                return Results[1]; // return the error code
            }
            if(Results[1] & 0x40){
                print_error("FLOPPY READ ERROR: DRIVER TOO SLOW (0x40)\n", 0);
                return Results[1]; // return the error code
            }
            if(Results[1] & 2){
                print_error("FLOPPY READ ERROR: DRIVE IS WRITE PROTECTED (0x02)\n", 0);
                return Results[1]; // return the error code
            }

            memcpy((void*)BufferAddress, (void*)FLOPPY_BOUNCE_BUFFER, 0x10000);
        }
        if(transfer_count % 0x10000 > 1){
            // program ISA-DMA
            ISA_DMA_Transfer_To(2, (uint32_t)FLOPPY_BOUNCE_BUFFER, (transfer_count % 0x10000) - 1, true, 0b01);

            // send the command
            FLOPPY_Send_Command(FLOPPY_CMD_READ, 8, Parameters);

            FLOPPY_Read_Result_Bytes(7, Results);

            // do error checking from st1 (Results[1])
            if(Results[1] & 0x80){
                print_error("FLOPPY READ ERROR: NOT ENOUGH SECTORS (0x80)\n", 0);
                return Results[1]; // return the error code
            }
            if(Results[1] & 0x40){
                print_error("FLOPPY READ ERROR: DRIVER TOO SLOW (0x40)\n", 0);
                return Results[1]; // return the error code
            }
            if(Results[1] & 2){
                print_error("FLOPPY READ ERROR: DRIVE IS WRITE PROTECTED (0x02)\n", 0);
                return Results[1]; // return the error code
            }

            memcpy((void*)BufferAddress, (void*)FLOPPY_BOUNCE_BUFFER, transfer_count % 0x10000);
        }
    }else{
        // program ISA-DMA
        ISA_DMA_Transfer_To(2, (uint32_t)FLOPPY_BOUNCE_BUFFER, (transfer_count) - 1, true, 0b01);

        // send the command
        FLOPPY_Send_Command(FLOPPY_CMD_READ, 8, Parameters);

        FLOPPY_Read_Result_Bytes(7, Results);

        // do error checking from st1 (Results[1])
        bool dirty = false;
        if(Results[1] & 0x80){
            print_error("FLOPPY READ ERROR: NOT ENOUGH SECTORS (0x80)\n", 0);
            return Results[1]; // return the error code
        }
        if(Results[1] & 0x40){
            print_error("FLOPPY READ ERROR: DRIVER TOO SLOW (0x40)\n", 0);
            return Results[1]; // return the error code
        }
        if(Results[1] & 2){
            print_error("FLOPPY READ ERROR: DRIVE IS WRITE PROTECTED (0x02)\n", 0);
            return Results[1]; // return the error code
        }

        memcpy((void*)BufferAddress, (void*)FLOPPY_BOUNCE_BUFFER, transfer_count);
    }

    #ifdef DEBUG
        SetTextColor(LCYAN, BLACK);
        printf("BUFFER ADDR: %x\n", (uint64_t)FLOPPY_BOUNCE_BUFFER);
        printf("ENDING CHS %i:%i:%i\n", Results[3], Results[4], Results[5]);
        printf("st0: %b\n", Results[0]);
        printf("st1: %b\n", Results[1]);
        printf("st2: %b\n", Results[2]);
        SetTextColor(WHITE, BLACK);
    #endif

    return 0;
}

/**
    * @brief Reads sectors from a FLOPPY by reprogramming the ISA-DMA and then issuing a READ command with it's parameters.
    This function uses a bounce buffer (at the top of this file) thats read into then memcpy'd into the real buffers
    location.
    * @param Drive The drive index
    * @param Cylinder The starting Cylinder to read from
    * @param Head The starting Head to read from
    * @param Sector The starting Sector to read from
    * @param BufferAddress the address of the buffer to write to
    * @param SectorCount The amount of sectors to read
    * @return int 0 on success, anything else is the error code
*/
int FLOPPY_Read_LBA(uint8_t Drive, uint64_t LBA, uint64_t BufferAddress, uint16_t SectorCount){
    CHS new = FLOPPY_LBA_To_CHS(LBA);
    return FLOPPY_Read_CHS(Drive, new.Cylinder, new.Head, new.Sector, BufferAddress, SectorCount);
}