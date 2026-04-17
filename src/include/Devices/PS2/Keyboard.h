#pragma once

#include "LowLevel/IDT.h"
#include "Devices/PIC.h"
#include "Devices/IO.h"
#include "Display/VGA.h"

#include "Libraries/std.h"
#include "LowLevel/IDT.h"
#include "stdint.h"

#include "Userland/Tasks.h"

extern bool should_proceed;
extern bool should_not_proceed;

enum PS2_KeyCodesPressed{
    NULL_CODE,
    ESCAPE, ONE, TWO, THREE, FOUR, FIVE, SIX, SEVEN, EIGHT, NINE, ZERO, MINUS, EQUALS, BACKSPACE, TAB, 
    Q, W, E, R, T, Y, U, I, O, P, OPEN_SQUARE_BRACKET, CLOSED_SQUARE_BRACKET,
    ENTER, L_CTRL, A, S, D, F, G, H, J, K, L, SEMICOLON, SINGLE_QUOTE, BACKTICK,
    L_SHIFT, BACKSLASH, Z, X, C, V, B, N, M, COMMA, PERIOD, FORWARDSLASH, R_SHIFT, KEYPAD_ASTERISK,
    LEFT_ALT, SPACE, CAPSLOCK, FU1, FU2, FU3, FU4, FU5, FU6, FU7, FU8, FU9, FU10, NUM_LOCK, SCROLL_LOCK,
    KEYPAD_7, KEYPAD_8, KEYPAD_9, KEYPAD_MINUS,
    KEYPAD_4, KEYPAD_5, KEYPAD_6, KEYPAD_PLUS,
    KEYPAD_1, KEYPAD_2, KEYPAD_3, KEYPAD_0, KEYPAD_PERIOD, FU11, FU12
};

extern void keyboard_stub();
void HandleKeyboardInterrupt(InterruptRegisters* regs);