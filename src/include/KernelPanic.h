#pragma once

#include "std.h"
#include "VGA.h"
#include "IDT.h"
#include "Power.h"
#include "PIC.h"

extern void kernel_panic_stub();
void KernelPanic(InterruptRegistersError* regs);