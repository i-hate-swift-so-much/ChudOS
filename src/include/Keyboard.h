#pragma once

#include "IDT.h"

extern "C" void keyboard_stub();
extern "C" void HandleKeyboardInterrupt(interrupt_frame* frame);