#pragma once

#include "Memory.h"
#include "IO.h"
#include "std.h"
#include "PIC.h"
#include "VGA.h"

struct DateData{
    uint16_t second;
    uint16_t minute;
    uint16_t hour;
    uint16_t day;
};

extern uint64_t SecondCount;
extern uint64_t TimerWindow ;

extern "C" void timer_interrupt_stub();
extern "C" void sync_time_stub();
extern "C" void TimerInterrupt(InterruptRegisters* frame);
extern "C" void SyncTime(InterruptRegisters* frame);
void SetTimerFrequency(uint16_t hz);