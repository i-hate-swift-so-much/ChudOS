#pragma once
#include <stddef.h>
#include "stdarg.h" // for variadic functions

extern int snapshotX;
extern int snapshotY;

void print(const char* toPrint, int length);
void cls();
void int_to_char_array(int n, char* buffer, size_t buffer_size, int padding);
void int_to_char_array_hex(int n, char* buffer, size_t buffer_size, int padding);
void int_to_char_array_binary(int n, char* buffer, size_t buffer_size, int padding);
void pad_string(int padding, char* buffer, int buffer_size, char filler, int start);
void print_centered(const char* toPrint, int length);
void setCursor(int x, int y);
void Scroll(int n);
void print_debug(const char* toPrint, int length);
void print_error(const char* toPrint, int length);
void print_success(const char* toPrint, int length);
void print_debug_snapshot(const char* toPrint, int length);
void print_error_snapshot(const char* toPrint, int length);
void print_success_snapshot(const char* toPrint, int length);
void take_print_snapshot();
void print_snapshot(const char* toPrint, int length);
void NewLine();
void printf(const char* toPrint, ...);