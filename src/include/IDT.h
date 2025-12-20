#pragma once

#include <stdint.h>

struct InterruptRegisters{
    uint64_t int_no;

    uint64_t cr2;

    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;
    uint64_t r11;
    uint64_t r10;
    uint64_t r9;
    uint64_t r8;

    uint64_t rdi;
    uint64_t rsi;
    uint64_t rbp;
    uint64_t rbx;
    uint64_t rdx;
    uint64_t rcx;
    uint64_t rax;

    // Pushed automatically by CPU:
    uint64_t eip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
}__attribute__((packed));

struct InterruptRegistersError{
    uint64_t int_no;
    
    uint64_t cr2;

    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;
    uint64_t r11;
    uint64_t r10;
    uint64_t r9;
    uint64_t r8;

    uint64_t rdi;
    uint64_t rsi;
    uint64_t rbp;
    uint64_t rbx;
    uint64_t rdx;
    uint64_t rcx;
    uint64_t rax;

    // Pushed automatically by CPU:
    uint64_t error_code;
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
}__attribute__((packed));

struct interrupt_frame {
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
} __attribute__((packed));

struct IDTEntry {
    uint16_t offset_low;    // Lower 16 bits of the handler's address
    uint16_t selector;      // Code segment selector
    uint8_t  ist;     // first 3 bits are the IST offset, rest MUST be zero
    uint8_t  flags;         // Type and attributes
    uint16_t offset_middle;   // Middle 16 bits of the handler's address
    uint32_t offset_high; // Upper 32 bits of the handler's address
    uint32_t zero; // must be zero
};

struct __attribute__((packed)) IDTR {
    uint16_t limit; // Size of IDT - 1
    uint64_t base;  // Base address of the IDT
};

void LoadIDT();
void SetIDTEntry(uint8_t entry_num, uint64_t handler_address, uint16_t selector, uint8_t flags);