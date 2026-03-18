#include "LowLevel/IDT.h"

#include "ErrorHandling/KernelPanic.h"

IDTEntry kernel_idt[256];

IDTR kernel_idtr_descriptor;

void LoadIDT(){
    kernel_idtr_descriptor.limit = (16 * 256) - 1;
    kernel_idtr_descriptor.base = (uint64_t)&kernel_idt;
    asm volatile("lidt %0" : : "m"(kernel_idtr_descriptor) : "memory");
}

void SetIDTEntry(uint8_t entry_num, uint64_t handler_address, uint16_t selector, uint8_t flags, uint8_t ist) {
    #ifdef DEBUG
        if(handler_address != (uint64_t)kernel_panic_stub){
            char test_char[64];
            int_to_char_array(entry_num, test_char, sizeof(test_char), 5);
            print_debug(test_char, 0);
            print_debug(":", 0);
            int_to_char_array_hex(handler_address, test_char, sizeof(test_char), 8);
            print_debug(test_char, 0);
            print_debug(":", 0);
            int_to_char_array(selector, test_char, sizeof(test_char), 8);
            print_debug(test_char, 0);
            print_debug(":", 0);
            int_to_char_array(flags, test_char, sizeof(test_char), 5);
            print_debug(test_char, 0);
            print_debug(":", 0);
            int_to_char_array(ist, test_char, sizeof(test_char), 5);
            print_debug(test_char, 0);
            print_debug("\n", 0);
        }
    #endif
    
    kernel_idt[entry_num].offset_low = handler_address & 0xFFFF;
    kernel_idt[entry_num].selector = selector;
    kernel_idt[entry_num].ist = ist & 0b111;
    kernel_idt[entry_num].flags = flags;
    kernel_idt[entry_num].offset_middle = (handler_address >> 16) & 0xFFFF;
    kernel_idt[entry_num].offset_high = (handler_address >> 32) & 0xFFFFFFFF;
    kernel_idt[entry_num].zero = 0;
}