#pragma once

#include "Memory.h"
#include "IO.h"
#include "std.h"
#include "PIC.h"
#include "VGA.h"

extern uint64_t SecondCount;

extern "C" void timer_interrupt_stub();
extern "C" void TimerInterrupt(RegistersKernelCall* frame);
void SetTimerFrequency(uint16_t hz);