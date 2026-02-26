#include <stdint.h>
#include "stddef.h"

#define KERNEL_LOAD 0x10000
#define KERNEL_NEW 0xffff800000100000

void memcpy(void* dest, void* src, size_t size){
    uint8_t* src2 = (uint8_t*)src;
    uint8_t* dest2 = (uint8_t*)dest;
    
    while(size--){
        dest2[size] = src2[size];
    }
}

void kernel_setup_main(uint64_t BootDrive){
    //copy the 64 kibibyte kernel to the new address
    memcpy((void*)KERNEL_NEW, (void*)KERNEL_LOAD, 0x30000);

    uint64_t thing = KERNEL_NEW;
    __asm__ volatile(
        "movq %0, %%rdi"
        "jmp *%1"
        :
        : "m"(BootDrive), "m" (thing)
        : "memory"
    );
}