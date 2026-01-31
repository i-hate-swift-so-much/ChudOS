#include "Display/VGA_E.h"

uint8_t VGA_Read_Index(uint8_t index){
    inb(0x3DA); // set port 0x3C0 to index mode
    outb(0x3C0, index);
    uint8_t ret = inb(0x3C1);
    outb(0x3C0, index);
    return ret;
}

void VGA_E_INIT(){

}