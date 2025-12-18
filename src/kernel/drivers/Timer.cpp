#include "Timer.h"

uint64_t SecondCount = 0;
uint64_t TimerWindow = 0;
uint16_t Frequency = 1193180;


extern "C" void TimerInterrupt(RegistersKernelCall* frame){
    char ticks_str[22];
    SecondCount = TimerWindow / 1000;
    afstd::int_to_char_array(SecondCount, ticks_str, sizeof(ticks_str));
    WriteString(ticks_str, 80-calculate_string_length(ticks_str), 0);
    TimerWindow++;

    pic_send_eoi(0x00);
}

void SetTimerFrequency(uint16_t hz) {
    uint32_t base_freq = 1193180; // PIT Base Clock
    uint16_t divisor = (uint16_t)(base_freq / hz);
    
    Frequency = hz;

    // Channel 0, Mode 3 (Square Wave), LSB/MSB
    outb(0x43, 0x36); 
    outb(0x40, divisor & 0xFF); // Low byte
    outb(0x40, (divisor >> 8) & 0xFF);   // High byte
}