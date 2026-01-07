#pragma once

#include "Memory.h"
#include "IO.h"
#include "std.h"
#include "IDT.h"
#include "PIC.h"
#include "VGA.h"

typedef struct{
    uint16_t second;
    uint16_t minute;
    uint16_t hour;
    uint16_t day;
} DateData;

extern uint64_t SecondsSinceBoot;
extern uint64_t TimerWindow;

void timer_interrupt_stub();
void sync_time_stub();
void TimerInterrupt(InterruptRegisters* frame);
void SyncTime(InterruptRegisters* frame);
void SetTimerFrequency(uint16_t hz);