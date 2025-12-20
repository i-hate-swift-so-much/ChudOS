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

    afstd::printf("\n");
    afstd::printf_centered("Made by Cameron McLaughlin under the third rendition");
    afstd::printf("\n");
    afstd::printf_centered("of the GNU General Public License (GPL 3.0)");
    afstd::printf("\n");
    afstd::printf_centered("In memory of King Terry Davis (1969-2018)");
    afstd::printf("\n\n");
    DrawBox(0, 0, 80, 5, "ChudOS");

    afstd::printf("Setting up interrupts\n");
    pic_remap(0x20, 0x28);
    for(int i = 0; i < 256; i++){
        SetIDTEntry(i, (uint64_t)kernel_panic_stub, 0x08, 0x8E);
    }
    afstd::printf("Succesfully Cleared IDT\n");
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
    afstd::printf("Succesfully set up Timer\n");
    SetIDTEntry(0x21, (uint64_t)keyboard_stub, 0x08, 0x8E);
    pic_unmask(0x01); // Keyboard
    afstd::printf("Succesfully set up keyboard\n");
    afstd::printf("Succesfully loaded IDT\n");
    SetTextColor(WHITE, BLACK);
    afstd::printf("Enabling Interrupts\n");
    asm volatile("sti");
    SetTextColor(LGREEN, BLACK);
    afstd::printf("Succesfully enabled interrupts\n");
    SetTextColor(WHITE, BLACK);
    afstd::printf("Setting up paging\n");

    InitMem();

    uint64_t* test_alloc = reinterpret_cast<uint64_t*>(malloc(&KernelTask));

    // made for testing paging
    *test_alloc = (uint64_t)0xFFULL;
    uint64_t test_read = *test_alloc;
    if(test_read == 0xFFULL){
        SetTextColor(LGREEN, BLACK);
        afstd::printf("Succesfully set up paging\n");
    }else{
        SetTextColor(LRED, BLACK);
        afstd::printf("Failed to set up paging, disabling system.\n");
        asm(
            "cli\n"
            "1:\n\t"
            "hlt\n"
            "jmp 1b\n"
            :::
        );
    }
    SetTextColor(WHITE, BLACK);

    afstd::printf("Press CTRL+ALT+ESC to force a kernel panic\n");
    afstd::printf("Press CTRL+ALT+TAB to force a page fault\n");
    afstd::printf("Press CTRL+ALT+LSHIFT to force an Invalid Opcode fault\n");
    afstd::printf("Press CTRL+ALT+BACKSPACE to force a general protection fault\n\n");
    DrawDivider(0, 18, 80);

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

    ClearScreenPixelMode(0x000000);
    RunDiagnostics();

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