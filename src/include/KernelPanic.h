#pragma once

#include "std.h"
#include "VBE.h"
#include "VGA.h"
#include "IDT.h"
#include "Power.h"
#include "PIC.h"

extern "C" void kernel_panic_stub();
extern "C" void KernelPanic(InterruptRegistersError* regs);