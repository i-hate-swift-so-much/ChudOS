#pragma once

#include "IDT.h"
#include "PIC.h"
#include "IO.h"
#include "VGA.h"

#include "std.h"
#include "KernelPanic.h"
#include "IDT.h"
#include "stdint.h"

extern bool should_proceed;
extern bool should_not_proceed;

extern void keyboard_stub();
void HandleKeyboardInterrupt(interrupt_frame* frame);