#include <stdint.h>
#include "stddef.h"

#define KERNEL_LOAD 0x80000
#define KERNEL_NEW 0x100000

void memcpy(void* dest, void* src, size_t size){
    uint8_t* src2 = (uint8_t*)src;
    uint8_t* dest2 = (uint8_t*)dest;
    
    while(size--){
        dest2[size] = src2[size];
    }
}

void kernel_setup_main(){
    //copy the 64 kibibyte kernel to the new address
    memcpy((void*)KERNEL_NEW, (void*)KERNEL_LOAD, 65024);

    uint64_t thing = KERNEL_NEW;
    __asm__ volatile(
        "jmp *%0"
        :
        : "m" (thing)
        : "memory"
    );
}