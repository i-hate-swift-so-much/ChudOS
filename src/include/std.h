#pragma once
#include <stddef.h>

namespace afstd{
    void printf(const char* toPrint, int length = 0);
    void cls();
    void int_to_char_array(int n, char* buffer, size_t buffer_size);
    void int_to_char_array_hex(int n, char* buffer, size_t buffer_size);
    void int_to_char_array_binary(int n, char* buffer, size_t buffer_size);
    void pad_string(int n, char* buffer, int buffer_size, char filler = '0', int start = 0);
}