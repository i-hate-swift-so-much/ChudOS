#include "Keyboard.h"

#include "PIC.h"
#include "IO.h"
#include "VGA.h"

#include "std.h"
#include "KernelPanic.h"
#include "IDT.h"
#include "stdint.h"

#define KBD_PORT 0x60

bool capslock = false;
bool upper = false;
bool ctrl = false;
bool alt = false;

uint8_t lastScancode = 0x00;

char scancode_map[59] = {
    0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b','\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',0,'a','s','d','f','g','h','j','k','l',';','\'','`',0,'\\','z','x','c','v','b','n','m',',','.','/',0,'*',0,' ',0
};

char scancode_upper[43] = {0,27,'!','@','#','$','%','^','&','*','(',')','_','+',0,0,0,0,0,0,0,0,0,0,0,0,'{','}',0,0,0,0,0,0,0,0,0,0,'~',':','"',0,'|'};

uintptr_t canonicalize_arithmetic(uintptr_t addr) {
    // This expression clears the top 16 bits, then arithmetically
    // extends bit 47 to bits 48-63 using a specific bitwise trick
    return (addr & ((1ULL << 48) - 1)) | ~(((addr & (1ULL << 47)) - 1));
}

void HandleKeyboardInterrupt(interrupt_frame* frame){
    uint8_t scancode = inb(KBD_PORT);

    bool is_character = false;

    if (scancode == 0xE0){
        lastScancode = scancode;
        pic_send_eoi(0x01);
        return;
    }

    char ch = 0;
    if(scancode == 0x2A || scancode == 0x36){
        upper = true;
    }else if(scancode == 0x3A){
        capslock = !capslock;
    }else if(scancode == 0xAA || scancode == 0xB6){
        upper = false;
    }else if(scancode == 0x1D || (scancode == 0x1D && lastScancode == 0xE0)){
        ctrl = true;
    }else if(scancode == 0x9D || (scancode == 0x9D && lastScancode == 0xE0)) {
        ctrl = false;
    }else if(scancode == 0x38){
        alt = true;
    }else if(scancode == 0xB8){
        alt = false;
    }else if(scancode < 59 && scancode != 0x01){
        ch = scancode_map[scancode];
        is_character = true;
    }else if(scancode == 0x1C){
        NewLine();
        return;
    }

    if(ch && is_character){
        if(capslock || upper && ch >= 97 && ch <=122){
            ch-=32;
        }
        if(upper && scancode_upper[scancode] != 0){
            ch = scancode_upper[scancode];
        }
        char str[2] = { ch, 0};
        printf(str, 0);
    }

    lastScancode = scancode;    

    pic_send_eoi(0x01);
}