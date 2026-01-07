#pragma once

#include "std.h"
#include "VGA.h"
#include "IDT.h"

#include <stdint.h>

extern void isr80_stub();
void handle_syscall(InterruptRegisters regs);