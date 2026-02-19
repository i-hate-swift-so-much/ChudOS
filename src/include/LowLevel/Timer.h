#pragma once

#include "LowLevel/Memory.h"
#include "Devices/IO.h"
#include "Libraries/std.h"
#include "LowLevel/IDT.h"
#include "Devices/PIC.h"
#include "Display/VGA.h"
#include "Userland/Tasks.h"

typedef struct{
    uint16_t second;
    uint16_t minute;
    uint16_t hour;
    uint16_t day;
} DateData;

extern uint64_t SecondsSinceBoot;
extern uint64_t TimerWindow;
extern uint16_t Frequency;
extern bool tasks_enabled;

void timer_interrupt_stub();
void sync_time_stub();
void TimerInterrupt(InterruptRegisters* frame);
void SyncTime(InterruptRegisters* frame);
void SetTimerFrequency(uint16_t hz);
void PrintCycles();
void PrintSecondsSinceBoot();
void context_switch(InterruptRegisters* regs);