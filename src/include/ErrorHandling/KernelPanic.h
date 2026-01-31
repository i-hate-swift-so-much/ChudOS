#pragma once

#include "Libraries/std.h"
#include "Display/VGA.h"
#include "LowLevel/IDT.h"
#include "LowLevel/Power.h"
#include "Devices/PIC.h"

extern void kernel_panic_stub();
void KernelPanic(InterruptRegistersError* regs);