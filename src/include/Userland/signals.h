#pragma once

#include "Tasks.h"
#include "LowLevel/Memory.h"

#include "stddef.h"
#include "stdint.h"
#include "stdbool.h"

void EstablishSignalStack(int pid);

void SignalReturn(int pid);

void SendSignal(int pid, enum SignalIdentifiers ident, uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3);
void RegisterSignal(int pid, enum SignalIdentifiers ident, uint64_t entry);