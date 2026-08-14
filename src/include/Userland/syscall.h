#pragma once

#include "Libraries/std.h"
#include "Display/VGA.h"
#include "LowLevel/IDT.h"
#include "Userland/Tasks.h"
#include "LowLevel/Timer.h"
#include "Userland/signals.h"
#include "Userland/wrappers.h"

#include <stdint.h>

struct sysinfo{
    size_t sys_mem_total;
    size_t sys_mem_avl;
    size_t sys_mem_used;
    size_t your_mem_size;
};

struct dent{
    char Name[128];
    size_t NameLen;
    uint8_t Flags;
};

extern void isr80_stub();
void handle_syscall(InterruptRegisters* regs);