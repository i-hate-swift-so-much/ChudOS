#pragma once

#include "std.h"
#include "VGA.h"
#include "IDT.h"
#include "Memory.h"

extern "C" void page_fault_stub();
extern "C" void HandlePageFault(InterruptRegistersError* regs);
extern "C" void GeneralProtectionFault(InterruptRegistersError* regs);
extern "C" void InvalidOpcode(InterruptRegistersError* regs);
extern "C" void gpf_stub();
extern "C" void invalid_opcode_stub();