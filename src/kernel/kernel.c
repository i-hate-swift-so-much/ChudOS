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

void kernel_startup_textmode(){
    cls();

    printf("\n", 0);
    printf_centered("Made by Cameron McLaughlin under the third rendition", 0);
    printf("\n", 0);
    printf_centered("of the GNU General Public License (GPL 3.0)", 0);
    printf("\n", 0);
    printf_centered("In memory of King Terry Davis (1969-2018)", 0);
    printf("\n\n", 0);
    DrawBox(0, 0, 80, 5, "ChudOS");

    printf("Setting up interrupts\n", 0);
    pic_remap(0x20, 0x28);
    for(int i = 0; i < 256; i++){
        SetIDTEntry(i, (uint64_t)kernel_panic_stub, 0x08, 0x8E);
    }
    printf("Succesfully Cleared IDT\n", 0);
    SetIDTEntry(0x06, (uint64_t)invalid_opcode_stub, 0x08, 0x8E);
    SetIDTEntry(0x0D, (uint64_t)gpf_stub, 0x08, 0x8E);
    SetIDTEntry(0x0E, (uint64_t)page_fault_stub, 0x08, 0x8E);
    SetIDTEntry(0x80, (uint64_t)isr80_stub, 0x08, 0x8E);
    SetTextColor(LGREEN, BLACK);
    SetIDTEntry(0x20, (uint64_t)timer_interrupt_stub, 0x08, 0x8E);
    SetIDTEntry(0x28, (uint64_t)sync_time_stub, 0x08, 0x8E);
    LoadIDT();
    SetTimerFrequency(1000); // the timer will go off every 1 milisecond
    outb(0x70, 0x8B);
    char previous = inb(0x71);
    outb(0x70, 0x8B);
    outb(0x71, previous | 0x40);
    pic_unmask(0x08); // for syncing time
    pic_unmask(0x00); // Timer
    printf("Succesfully set up Timer\n", 0);
    SetIDTEntry(0x21, (uint64_t)keyboard_stub, 0x08, 0x8E);
    pic_unmask(0x01); // Keyboard
    printf("Succesfully set up keyboard\n", 0);
    printf("Succesfully loaded IDT\n", 0);
    SetTextColor(WHITE, BLACK);
    printf("Enabling Interrupts\n", 0);
    asm volatile("sti");
    SetTextColor(LGREEN, BLACK);
    printf("Succesfully enabled interrupts\n", 0);
    SetTextColor(WHITE, BLACK);
    printf("Setting up paging\n", 0);

    InitMem();

    uint64_t* test_alloc = (uint64_t*)(malloc(&KernelTask));

    // made for testing paging
    *test_alloc = (uint64_t)0xFFULL;
    uint64_t test_read = *test_alloc;
    if(test_read == 0xFFULL){
        SetTextColor(LGREEN, BLACK);
        printf("Succesfully set up paging\n", 0);
    }else{
        SetTextColor(LRED, BLACK);
        printf("Failed to set up paging, disabling system.\n", 0);
        asm(
            "cli\n"
            "1:\n\t"
            "hlt\n"
            "jmp 1b\n"
            :::
        );
    }
    SetTextColor(WHITE, BLACK);

    printf("Press CTRL+ALT+ESC to force a kernel panic\n", 0);
    printf("Press CTRL+ALT+TAB to force a page fault\n", 0);
    printf("Press CTRL+ALT+LSHIFT to force an Invalid Opcode fault\n", 0);
    printf("Press CTRL+ALT+BACKSPACE to force a general protection fault\n\n", 0);
    DrawDivider(0, 18, 80, "");

    while (1){

    }
}

void kernel_main(){ 
    kernel_startup_textmode();
    return;
}