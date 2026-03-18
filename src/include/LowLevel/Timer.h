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

extern volatile uint64_t SecondsSinceBoot;
extern volatile uint64_t TimerWindow;
extern volatile uint16_t Frequency;

void timer_interrupt_stub();
void sync_time_stub();
void task_switch_frame(InterruptRegisters* dest, InterruptRegisters* src);
void TimerInterrupt(InterruptRegisters* frame);
void SyncTime(InterruptRegisters* frame);
void SetTimerFrequency(uint16_t hz);
void PrintCycles();
void PrintSecondsSinceBoot();
void context_switch(InterruptRegisters* regs);
void ForceSwitch(InterruptRegisters* regs);
void EnableTasks();