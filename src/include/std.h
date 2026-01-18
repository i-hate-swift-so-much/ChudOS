#pragma once
#include <stddef.h>

void printf(const char* toPrint, int length);
void cls();
void int_to_char_array(int n, char* buffer, size_t buffer_size, int padding);
void int_to_char_array_hex(int n, char* buffer, size_t buffer_size, int padding);
void int_to_char_array_binary(int n, char* buffer, size_t buffer_size, int padding);
void pad_string(int padding, char* buffer, int buffer_size, char filler, int start);
void printf_centered(const char* toPrint, int length);
void setCursor(int x, int y);
void Scroll(int n);
void NewLine();