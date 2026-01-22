#include "Keyboard.h"

#define KBD_PORT 0x60

bool capslock = false;
bool upper = false;
bool ctrl = false;
bool alt = false;

bool should_proceed = false;
bool should_not_proceed = false;

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

    if(scancode == 0x1C || scancode == 0x9C){
        should_proceed = true;
        should_not_proceed = false;
        return;
    }else{
        should_not_proceed = true;
        should_proceed = false;
        return;
    }

    pic_send_eoi(0x01);
}