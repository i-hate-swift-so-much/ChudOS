#pragma once
#include <stddef.h>
#include "stdarg.h" // for variadic functions

extern int snapshotX;
extern int snapshotY;

void printf(const char* toPrint, int length);
void cls();
void int_to_char_array(int n, char* buffer, size_t buffer_size, int padding);
void int_to_char_array_hex(int n, char* buffer, size_t buffer_size, int padding);
void int_to_char_array_binary(int n, char* buffer, size_t buffer_size, int padding);
void pad_string(int padding, char* buffer, int buffer_size, char filler, int start);
void printf_centered(const char* toPrint, int length);
void setCursor(int x, int y);
void Scroll(int n);
void printf_debug(const char* toPrint, int length);
void printf_error(const char* toPrint, int length);
void printf_success(const char* toPrint, int length);
void printf_debug_snapshot(const char* toPrint, int length);
void printf_error_snapshot(const char* toPrint, int length);
void printf_success_snapshot(const char* toPrint, int length);
void take_printf_snapshot();
void printf_snapshot(const char* toPrint, int length);
void NewLine();
void printf_variable(const char* toPrint, ...);