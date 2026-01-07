#pragma once

#include "IDT.h"

extern void keyboard_stub();
void HandleKeyboardInterrupt(interrupt_frame* frame);