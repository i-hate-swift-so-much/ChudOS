#include "Timer.h"

uint64_t SecondCount = 0;
uint64_t BiosSecondCount = 0;
uint64_t TimerWindow = 0;
uint16_t Frequency = 1193180;
bool enabled = false;

bool IsLeapYear(int y) {
    return (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0);
}

DateData ParseSeconds(uint64_t time){
    DateData ret;

    ret.second = (uint16_t)(time % 60);
    time /= 60;
    ret.minute = (uint16_t)(time % 60);
    time /= 60;
    ret.hour = (uint16_t)(time % 60);
    time /= 24;

    ret.day = (uint16_t)time;

    return ret;
}

extern "C" void TimerInterrupt(InterruptRegisters* frame){
    if(!enabled){ pic_send_eoi(0x00); return; }
    
    char ticks_str[22];
    SecondCount = TimerWindow / 1000;
    DateData date = ParseSeconds(SecondCount+BiosSecondCount);
    afstd::int_to_char_array(date.second, ticks_str, sizeof(ticks_str));
    WriteString(ticks_str, 79-calculate_string_length(ticks_str), 1);
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

extern "C" void SyncTime(InterruptRegisters* frame){
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hours;
    uint8_t day;

    outb(0x70, (1 << 7) | 0x00);
    seconds = inb(0x71);

    outb(0x70, (1 << 7) | 0x02);
    minutes = inb(0x71);

    outb(0x70, (1 << 7) | 0x04);
    hours = inb(0x71);

    outb(0x70, (1 << 7) | 0x07);
    day = inb(0x71);


    BiosSecondCount+=(uint64_t)seconds;
    BiosSecondCount+=(uint64_t)minutes*60;
    BiosSecondCount+=(uint64_t)hours*(60*60);
    BiosSecondCount+=(uint64_t)day*86400;

    pic_send_eoi(0x08);
    pic_mask(0x08);

    enabled = true;
}