#include "std.h"
#include "IDT.h"
#include "PIC.h"
#include "Keyboard.h"
#include "VGA.h"
#include "IO.h"
#include "Power.h"
#include "syscall.h"
#include "Memory.h"
#include "Timer.h"
#include "KernelPanic.h"
#include "Exceptions.h"
#include "PCI.h"
#include "AHCI.h"
#include "GDT.h"

void DrawWelcome(){
    printf("\n", 0);
    printf_centered("Made by Cameron McLaughlin under the third rendition", 0);
    printf("\n", 0);
    printf_centered("of the GNU General Public License (GPL 3.0)", 0);
    printf("\n", 0);
    printf_centered("In memory of King Terry Davis (1969-2018)", 0);
    printf("\n\n", 0);
    DrawBox(0, 0, 80, 5, "ChudOS");
}

void kernel_startup(){
    cls();

    PrintCycles();
    printf(" Enumerating Memory\n", 0);

    Enumerate_E820();

    SetTextColor(LGREEN, BLACK);
    printf("SUCCESS\n", 0);
    SetTextColor(WHITE, BLACK);
    PrintCycles();
    printf(" Loading IDT ", 0);
    pic_remap(0x20, 0x28);
    for(int i = 0; i < 256; i++){
        SetIDTEntry(i, (uint64_t)kernel_panic_stub, 0x08, 0x8E, 0x04);
    }
    SetIDTEntry(0x06, (uint64_t)invalid_opcode_stub, 0x08, 0x8F, 0x04);
    SetIDTEntry(0x0D, (uint64_t)gpf_stub, 0x08, 0x8F, 0x04);
    SetIDTEntry(0x0E, (uint64_t)page_fault_stub, 0x08, 0x8F, 0x04);
    SetIDTEntry(0x80, (uint64_t)isr80_stub, 0x08, 0x8E, 0x03);
    SetIDTEntry(0x20, (uint64_t)timer_interrupt_stub, 0x08, 0x8E, 0x01);
    SetIDTEntry(0x28, (uint64_t)sync_time_stub, 0x08, 0x8E, 0x01);
    LoadIDT();
    SetTimerFrequency(1000); // the timer will go off every 1 milisecond
    outb(0x70, 0x8B);
    char previous = inb(0x71);
    outb(0x70, 0x8B);
    outb(0x71, previous | 0x40);
    pic_unmask(0x08); // for syncing time
    SetIDTEntry(0x21, (uint64_t)keyboard_stub, 0x08, 0x8E, 0x00);
    pic_unmask(0x01); // Keyboard
    SetTextColor(LGREEN, BLACK);
    printf("SUCCESS\n", 0);
    SetTextColor(WHITE, BLACK);
    PrintCycles();
    printf(" Enabling Interrupts ", 0);
    asm volatile("sti");
    SetTextColor(LGREEN, BLACK);
    printf("SUCCESS\n", 0);
    SetTextColor(WHITE, BLACK);
    PrintCycles();
    printf(" Setting up paging ", 0);

    InitMem();

    uint64_t* test_alloc = (uint64_t*)(malloc(&KernelTask));

    // made for testing paging
    *test_alloc = (uint64_t)0xFFULL;
    uint64_t test_read = *test_alloc;
    SetTextColor(LGREEN, BLACK);
    printf("SUCCESS\n", 0);
    SetTextColor(WHITE, BLACK);

    PrintCycles();
    printf(" Scanning Busses ", 0);
    ScanBusses();
    SetTextColor(LGREEN, BLACK);
    printf("SUCCESS\n", 0);
    SetTextColor(WHITE, BLACK);

    PrintCycles();
    printf(" Detecting drive ", 0);

    if(AHCI_Controller == NULL){ 
        SetTextColor(LRED, BLACK);
        printf("FAIL\n", 0); 
    }else{
        SetTextColor(LGREEN, BLACK);
        printf("SUCCESS ", 0);
        SetTextColor(WHITE, BLACK);
        char Vendor_ID[22];
        char Device_ID[22];
        int_to_char_array_hex(AHCI_Controller->Header.VendorID, Vendor_ID, sizeof(Vendor_ID), 0);
        int_to_char_array_hex(AHCI_Controller->Header.DeviceID, Device_ID, sizeof(Device_ID), 0);
        printf("ID: ", 0);
        printf(Vendor_ID, 0);
        printf(":", 0);
        printf(Device_ID, 0);
        printf("\n", 0);
        PrintCycles();
        printf(" Initializing Drive ", 0);
        AHCI_Init(AHCI_Controller);
        if(AHCI_Ownership){ 
            SetTextColor(LGREEN, BLACK);
            printf("SUCCESS ", 0);
        }else{
            SetTextColor(LRED, BLACK);
            printf("FAIL ", 0);
        }
    }
    SetTextColor(WHITE, BLACK);

    while (1){

    }
}

void kernel_main(){ 
    kernel_startup();
    return;
}