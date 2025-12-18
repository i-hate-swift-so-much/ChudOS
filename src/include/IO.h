#pragma once

#include <stdint.h>
#include <stdbool.h>

static inline void outb(uint16_t port, uint8_t value){
    asm volatile("outb %b0, %1"
                :
                : "a"(value), "Nd"(port)
                :);
}

static inline uint8_t inb(uint16_t port){
    uint8_t ret;
    asm volatile("inb %1, %0"
                : "=a"(ret)
                : "Nd"(port));
    return ret;
}

static inline void outl(uint16_t port, uint32_t value){
    asm volatile("outl %b0, %1"
                :
                : "a"(value), "Nd"(port)
                :);
}

static inline uint8_t inl(uint16_t port){
    uint32_t ret;
    asm volatile("inl %1, %0"
                : "=a"(ret)
                : "Nd"(port));
    return ret;
}

static inline void io_wait(){
    asm volatile("outb %0, $0x80" : : "a"((uint8_t)0));
}