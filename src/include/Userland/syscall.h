#pragma once

#include "Libraries/std.h"
#include "Display/VGA.h"
#include "LowLevel/IDT.h"

#include <stdint.h>

extern void isr80_stub();
void handle_syscall(InterruptRegisters regs);