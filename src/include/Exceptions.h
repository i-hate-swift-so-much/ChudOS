#pragma once

#include "std.h"
#include "VGA.h"
#include "IDT.h"
#include "Memory.h"

void HandlePageFault(InterruptRegistersError* regs);
void GeneralProtectionFault(InterruptRegistersError* regs);
void InvalidOpcode(InterruptRegistersError* regs);
extern void gpf_stub();
extern void invalid_opcode_stub();
extern void page_fault_stub();