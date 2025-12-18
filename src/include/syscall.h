#pragma once

#include "std.h"
#include "VGA.h"
#include "IDT.h"

#include <stdint.h>

extern "C" void isr80_stub();
extern "C" void handle_syscall(RegistersUsersCall regs);