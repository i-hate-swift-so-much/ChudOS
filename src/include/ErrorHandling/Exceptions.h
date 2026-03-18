#pragma once

#include "Libraries/std.h"
#include "Display/VGA.h"
#include "LowLevel/IDT.h"
#include "LowLevel/Memory.h"
#include "Userland/Tasks.h"

void HandlePageFault(InterruptRegistersError* regs);
void GeneralProtectionFault(InterruptRegistersError* regs);
void InvalidOpcode(InterruptRegistersError* regs);
extern void gpf_stub();
extern void invalid_opcode_stub();
extern void page_fault_stub();