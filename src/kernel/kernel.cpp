#include "std.h"
#include "IDT.h"
#include "PIC.h"
#include "Keyboard.h"
#include "VGA.h"
#include "IO.h"
#include "Power.h"
#include "syscall.h"
#include "VBE.h"
#include "KernelPanic.h"
#include "Memory.h"
#include "Timer.h"
#include "Exceptions.h"

void kernel_startup_textmode(VBEModeInfoBlock* vbe_info){
    VBEModeInfoBlock dereference = *vbe_info;
    
    SetMainVBE(vbe_info);
    afstd::cls();
    afstd::printf("Entering the City of Dis\n");
    afstd::printf("+------------------------------------------------------+\n");
    afstd::printf("| Made by Cameron McLaughlin under the third rendition |\n|     of the GNU General Public License (GPL 3.0)      |\n|      In memory of King Terry Davis (1969-2018)       |\n");
    afstd::printf("+------------------------------------------------------+\n");
    afstd::printf("Remapping PIC...\n");
    pic_remap(0x20, 0x28);
    afstd::printf("Setting up IDT...\n");
    for(int i = 0; i < 256; i++){
        SetIDTEntry(i, (uint64_t)kernel_panic_stub, 0x08, 0x8E);
    }
    afstd::printf("Succesfully Cleared IDT \n");
    SetIDTEntry(0x06, (uint64_t)invalid_opcode_stub, 0x08, 0x8E);
    SetIDTEntry(0x0D, (uint64_t)gpf_stub, 0x08, 0x8E);
    SetIDTEntry(0x0E, (uint64_t)page_fault_stub, 0x08, 0x8E);
    SetIDTEntry(0x80, (uint64_t)isr80_stub, 0x08, 0x8E);
    SetTextColor(GREEN, BLACK);
    SetIDTEntry(0x20, (uint64_t)timer_interrupt_stub, 0x08, 0x8E);
    SetTimerFrequency(1000); // the timer will go off every 1 milisecond
    pic_unmask(0x00); // Timer
    afstd::printf("Succesfully set up Timer\n");
    SetIDTEntry(0x21, (uint64_t)HandleKeyboardInterrupt, 0x08, 0x8E);
    pic_unmask(0x01); // Keyboard
    afstd::printf("Succesfully set up keyboard\n");
    LoadIDT();
    afstd::printf("Succesfully loaded IDT\n");
    SetTextColor(WHITE, BLACK);
    afstd::printf("Enabling Interrupts\n");
    asm volatile("sti");
    SetTextColor(GREEN, BLACK);
    afstd::printf("Succesfully enabled interrupts\n");
    SetTextColor(WHITE, BLACK);
    afstd::printf("Done!\n");

    InitMem();
    
    char test_char[22];
    afstd::int_to_char_array(mem_GetBit(0, 16, 1), test_char, sizeof(test_char));
    afstd::printf("Mem (0,0,15,0): ");
    afstd::printf(test_char);
    afstd::printf("\n");

    uint64_t* test_alloc = reinterpret_cast<uint64_t*>(malloc(&KernelTask));

    // made for testing paging
    *test_alloc = (uint64_t)0xFFULL;
    uint64_t test_read = *test_alloc;
    if(test_read == 0xFFULL){
        SetTextColor(GREEN, BLACK);
        afstd::printf("If you're reading this, paging is correctly set up.\n");
    }else{
        SetTextColor(LRED, BLACK);
        afstd::printf("If you're reading this, paging is incorrectly set up.\n");
    }
    SetTextColor(WHITE, BLACK);

    while (1){

    }
}

static uint32_t test_image_data[100] = {
        0xffffff, 0x0000ff, 0x0000ff, 0x0000ff, 0x0000ff, 0x0000ff, 0x0000ff, 0x0000ff, 0x0000ff, 0x0000ff,
        0x0000ff, 0x0000ff, 0x0000ff, 0x0000ff, 0x0000ff, 0x0000ff, 0x0000ff, 0x0000ff, 0x0000ff, 0x0000ff,
        0x0000ff, 0x0000ff, 0x0000ff, 0x0000ff, 0x0000ff, 0x0000ff, 0x0000ff, 0x0000ff, 0x0000ff, 0x0000ff,
        0x0000ff, 0x0000ff, 0x0000ff, 0x0000ff, 0x0000ff, 0x0000ff, 0x0000ff, 0x0000ff, 0x0000ff, 0x0000ff,
        0x0000ff, 0x0000ff, 0x0000ff, 0x0000ff, 0x0000ff, 0x0000ff, 0x0000ff, 0x0000ff, 0x0000ff, 0x0000ff,
        0x0000ff, 0x0000ff, 0x0000ff, 0x0000ff, 0x0000ff, 0x0000ff, 0x0000ff, 0x0000ff, 0x0000ff, 0x0000ff,
        0x0000ff, 0x0000ff, 0x0000ff, 0x0000ff, 0x0000ff, 0x0000ff, 0x0000ff, 0x0000ff, 0x0000ff, 0x0000ff,
        0x0000ff, 0x0000ff, 0x0000ff, 0x0000ff, 0x0000ff, 0x0000ff, 0x0000ff, 0x0000ff, 0x0000ff, 0x0000ff,
        0x0000ff, 0x0000ff, 0x0000ff, 0x0000ff, 0x0000ff, 0x0000ff, 0x0000ff, 0x0000ff, 0x0000ff, 0x0000ff,
        0x0000ff, 0x0000ff, 0x0000ff, 0x0000ff, 0x0000ff, 0x0000ff, 0x0000ff, 0x0000ff, 0x0000ff, 0x0000ff
    };

void kernel_startup_pixelmode(VBEModeInfoBlock* vbe_info){
    SetMainVBE(vbe_info);
    pic_remap(0x20, 0x28);
    for(int i = 0; i < 256; i++){
        SetIDTEntry(i, (uint64_t)kernel_panic_stub, 0x08, 0x8E);
    }
    SetIDTEntry(0x80, (uint64_t)isr80_stub, 0x08, 0x8E);
    SetIDTEntry(0x21, (uint64_t)HandleKeyboardInterrupt, 0x08, 0x8E);
    pic_unmask(0x01); // IRQ1
    //ClearScreenPixelMode(0x000000);
    //RunDiagnostics();

    while (1){

    }
}

extern "C" void kernel_main(VBEModeInfoBlock* vbe_info, uint32_t vbe_present){ 
    if(vbe_present){
        kernel_startup_pixelmode(vbe_info);
    }else{
        kernel_startup_textmode(vbe_info);
    }
    return;
}