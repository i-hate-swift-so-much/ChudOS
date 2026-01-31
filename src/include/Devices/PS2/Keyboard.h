#pragma once

#include "LowLevel/IDT.h"
#include "Devices/PIC.h"
#include "Devices/IO.h"
#include "Display/VGA.h"

#include "Libraries/std.h"
#include "LowLevel/IDT.h"
#include "stdint.h"

extern bool should_proceed;
extern bool should_not_proceed;

extern void keyboard_stub();
void HandleKeyboardInterrupt(interrupt_frame* frame);