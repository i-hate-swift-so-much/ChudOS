#pragma once
#include "IO.h"

// Defines the structure of the Local APIC's (Advanced Programmable Interrupt Controller)
// registers so I dont have to do math.
struct APIC_Registers{
    volatile uint32_t LAPIC_ID;
    volatile uint32_t LAPIC_Version;
    volatile uint32_t Task_Priority;
    volatile uint32_t Arbitration_Priority;
    volatile uint32_t Processor_Priority;
    volatile uint32_t EOI;
    volatile uint32_t Remote_Read;
    volatile uint32_t Logical_Destination;
    volatile uint32_t Destination_Format;
    volatile uint32_t Spurious_Interrupt_Vector;
    volatile uint32_t ISR_1;
    volatile uint32_t ISR_2;
    volatile uint32_t ISR_3;
    volatile uint32_t ISR_4;
    volatile uint32_t ISR_5;
    volatile uint32_t ISR_6;
    volatile uint32_t ISR_7;
    volatile uint32_t ISR_8;

};