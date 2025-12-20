#pragma once
#include <stddef.h>

namespace afstd{
    void printf(const char* toPrint, int length = 0);
    void cls();
    void int_to_char_array(int n, char* buffer, size_t buffer_size);
    void int_to_char_array_hex(int n, char* buffer, size_t buffer_size, int padding = 0);
    void int_to_char_array_binary(int n, char* buffer, size_t buffer_size, int padding = 0);
    void pad_string(int padding, char* buffer, int buffer_size, char filler = '0', int start = 0);
    void printf_centered(const char* toPrint, int length = 0);
    void setCursor(int x, int y);
}