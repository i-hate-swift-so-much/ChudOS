#include "IDT.h"

IDTEntry kernel_idt[256];

IDTR kernel_idtr_descriptor;

void LoadIDT(){
    kernel_idtr_descriptor.limit = (sizeof(IDTEntry) * 256) - 1;
    kernel_idtr_descriptor.base = (uint64_t)&kernel_idt;
    asm volatile("lidt %0" : : "m"(kernel_idtr_descriptor));
}

void SetIDTEntry(uint8_t entry_num, uint64_t handler_address, uint16_t selector, uint8_t flags) {
    kernel_idt[entry_num].offset_low = handler_address & 0xFFFF;
    kernel_idt[entry_num].selector = selector;
    kernel_idt[entry_num].ist = 0;
    kernel_idt[entry_num].flags = flags;
    kernel_idt[entry_num].offset_middle = (handler_address >> 16) & 0xFFFF;
    kernel_idt[entry_num].offset_high = (handler_address >> 32) & 0xFFFFFFFF;
    kernel_idt[entry_num].zero = 0;
}