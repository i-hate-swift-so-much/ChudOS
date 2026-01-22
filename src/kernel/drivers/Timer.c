#include "Timer.h"

#define BASE_FREQUENCY 1193180

uint64_t SecondsSinceBoot = 0;
uint64_t BiosTime = 0;
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

void PrintCycles(){
    printf("[", 0);
    char cycles[36];
    int_to_char_array(TimerWindow, cycles, sizeof(cycles), 10);
    printf(cycles, 0);
    printf("]", 0);
}

void PrintSecondsSinceBoot(){
    printf("[", 0);
    char cycles[36];
    int_to_char_array(SecondsSinceBoot, cycles, sizeof(cycles), 10);
    printf(cycles, 0);
    printf("]", 0);
}

void TimerInterrupt(InterruptRegisters* frame){    
    TimerWindow++;
    if(!enabled){ pic_send_eoi(0x00); return; }
    
    TimerWindow %= 1000;
    if(TimerWindow == 999){SecondsSinceBoot++;}
    
    pic_send_eoi(0x00);
}

void SetTimerFrequency(uint16_t hz) {
    uint16_t divisor = (uint16_t)(BASE_FREQUENCY / hz);
    
    Frequency = hz;

    // Channel 0, Mode 3 (Square Wave), LSB/MSB
    outb(0x43, 0x36); 
    outb(0x40, divisor & 0xFF); // Low byte
    outb(0x40, (divisor >> 8) & 0xFF);   // High byte
}

void SyncTime(InterruptRegisters* frame){
    BiosTime = 0;
    SecondsSinceBoot = 0;
    
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


    BiosTime+=(uint64_t)seconds;
    BiosTime+=(uint64_t)minutes*60;
    BiosTime+=(uint64_t)hours*(60*60);
    BiosTime+=(uint64_t)day*86400;

    pic_send_eoi(0x08);
    pic_mask(0x08);
    pic_unmask(0x00);

    enabled = true;
}