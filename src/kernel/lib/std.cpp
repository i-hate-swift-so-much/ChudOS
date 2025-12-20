#include "std.h"
#include "Math.h"
#include "VGA.h"


int curX = 0;
int curY = 0;

namespace afstd{
    void printf(const char* toPrint, int length){
        if(length == 0){ length = calculate_string_length(toPrint); }
        for (int i = 0; i < length; i++){
            
            if(curX > 79){ curX = 0; curY++; }
            if(curX == 79 && curY == 24) {curX = 0; curY = 0; ClearScreen(); }
            char curChar = toPrint[i];
            if(curChar == ' '){
                char curWordScan;
                int nextWordLength = 0;
                for(int b = i+1; b < length; b++){
                    curWordScan = toPrint[b];
                    nextWordLength++;
                    if(curWordScan == ' '){
                        break;
                    }
                }
                if(nextWordLength+curX > 79){
                    if(curY == 25){
                        afstd::cls();
                    }else{
                        curX = 0;
                        curY++;
                    }
                }else{
                    curX++;
                }
                //curX++;
            }else if(curChar == '\n'){
                if(curY == 25){
                    char temp_char[22];
                    afstd::cls();
                }else{
                    curX = 0;
                    curY++;
                }
            }else if(curChar == '\0'){
                break;
            }else if(curChar == '\b'){
                if(curX == 0 && curY == 0){ continue; }
                if(curX == 0){
                    // search for the first empty char
                    curY--;
                    for(int i = 0; i < 80; i++){
                        unsigned char scan = ReadCharacter(i, curY);
                        unsigned char nextScan = ReadCharacter(i+1, curY);
                        unsigned char breaker = ' ';
                        if(scan == breaker && nextScan == breaker){
                            curX = i;
                            break;
                        }else if (scan == breaker && i == 79)
                        {
                            curX = i;
                            break;
                        }
                        
                        curX = 79;
                    }
                }else{
                    curX--;
                }
                WriteCharacter(' ', curX, curY);
            }else if(curChar == '\t'){
                if(curX > 79-8 && curY == 24){
                    afstd::cls();
                    curX = 8-(79-curX);
                    curY = 0;
                }else if(curX > 79-8){
                    curY++;
                    curX = 8-(79-curX);
                }else{
                    curX+=8;
                }
            }else{
                WriteCharacter(curChar, curX, curY);
                if(curX == 79 && curY == 24){
                    afstd::cls();
                    curX = 0;
                    curY = 0;
                }else if(curX == 79){
                    curX = 0;
                    curY++;
                }else{
                    curX++;
                }
            }
        }
    }
    void cls(){
        curX = 0;
        curY = 0;
        ClearScreen();
    }
    void int_to_char_array(int n, char* buffer, size_t buffer_size) {
        bool negative = n < 0;
        
        n = abs(n);
        
        if (n == 0) {
            buffer[0] = '0';
            buffer[1] = '\0';
            return;
        }

        int temp = n;
        int digits = 0;
        while (temp != 0) {
            temp /= 10;
            digits++;
        }

        if (buffer_size < digits){
            return;
        }


        int lastI;
        // Extract digits from right to left and convert to ASCII characters
        if(negative){
            // Add null terminator at the end
            buffer[digits+1] = '\0';

            // Convert with negative symbol
            for (int i = digits - 1; i >= 0; i--) {
                char temp_char = (char)((n % 10) + '0');
                buffer[i+1] = temp_char;
                n /= 10; // divide n by 10
                lastI = i;
            }
            buffer[lastI] = '-';
        }else{
            // Add null terminator at the end
            buffer[digits] = '\0';

            // Convert without negative symbol
            for (int i = digits - 1; i >= 0; i--) {
                char temp_char = (char)((n % 10) + '0');
                buffer[i] = temp_char;
                n /= 10; // divide n by 10
                lastI = i;
            }
        }
    }
    void int_to_char_array_hex(int n, char* buffer, size_t buffer_size, int padding) {
        bool negative = n < 0;
        
        n = abs(n);
        
        if (n == 0 && padding == 0) {
            buffer[0] = '0';
            buffer[1] = 'x';
            buffer[2] = '0';
            buffer[3] = '\0';
            return;
        }else if(n == 0 && padding != 0){
            buffer[0] = '0';
            buffer[1] = 'x';
            buffer[2] = '0';
            buffer[3] = '\0';
            afstd::pad_string(padding, buffer, buffer_size, '0', 2);
            return;
        }

        int temp = n;
        int digits = 0;
        while (temp > 0) {
            temp /= 16;
            digits++;
        }

        size_t required_size = digits + 2 + (negative ? 1 : 0) + 1;
        if (buffer_size < digits){
            return;
        }

        int lastI;
        // Extract digits from right to left and convert to ASCII characters
        if(negative){
            // Add null terminator at the end
            buffer[digits+1] = '\0';

            // Convert with negative symbol
            for (int i = digits - 1; i >= 0; i--) {
                int remainder = n % 16;
                char temp_char;
                if(remainder < 10){
                    temp_char = (char)(remainder + '0');
                }else{
                    temp_char = (char)((remainder - 10) + 'A'); // This does remainder-10 because it's just an offset from A, and if the number was something like 15 it would be 15 + 'A' = P.
                }
                buffer[i+3] = temp_char;
                n /= 16;
                lastI = i;
            }
            buffer[lastI] = '-';
            buffer[lastI+1] = '0';
            buffer[lastI+2] = 'x';
            afstd::pad_string(padding, buffer, buffer_size, '0', 3);
        }else{
            // Convert without negative symbol
            for (int i = digits - 1; i >= 0; i--) {
                int remainder = n % 16;
                char temp_char;
                if(remainder < 10){
                    temp_char = (char)(remainder + '0');
                }else{
                    temp_char = (char)((remainder - 10) + 'A');
                }
                buffer[i+2] = temp_char;
                n /= 16;
                lastI = i;
            }
            buffer[lastI] = '0';
            buffer[lastI+1] = 'x';
            afstd::pad_string(padding, buffer, buffer_size, '0', 2);
        }
    }
    void int_to_char_array_binary(int n, char* buffer, size_t buffer_size, int padding){
        bool negative = n < 0;

        n = abs(n);

        if (n == 0 && padding == 0) {
            buffer[0] = '0';
            buffer[1] = 'b';
            buffer[2] = '0';
            buffer[3] = '\0';
            return;
        }else if(n == 0 && padding != 0){
            buffer[0] = '0';
            buffer[1] = 'b';
            buffer[2] = '0';
            buffer[3] = '\0';
            afstd::pad_string(padding, buffer, buffer_size, '0', 2);
            return;
        }

        int temp = n;
        int digits = 0;
        while(temp > 0){
            temp /= 2;
            digits++;
        }

        if(buffer_size < digits){ return; }

        int lastI;
        // Extract digits from right to left and convert to ASCII characters
        if(negative){
            // Add null terminator at the end
            buffer[digits+1] = '\0';

            // Convert with negative symbol
            for (int i = digits - 1; i >= 0; i--) {
                int remainder = n % 2;
                char temp_char;
                temp_char = (char)(remainder + '0');
                buffer[i+3] = temp_char;
                n /= 2;
                lastI = i;
            }
            buffer[lastI] = '-';
            buffer[lastI+1] = '0';
            buffer[lastI+2] = 'b';
            afstd::pad_string(padding, buffer, buffer_size, '0', 3);
        }else{
            // Convert without negative symbol
            for (int i = digits - 1; i >= 0; i--) {
                int remainder = n % 2;
                char temp_char;
                temp_char = (char)(remainder + '0');
                buffer[i+2] = temp_char;
                n /= 2;
                lastI = i;
            }
            buffer[lastI] = '0';
            buffer[lastI+1] = 'b';
            afstd::pad_string(padding, buffer, buffer_size, '0', 2);
        }
    }
    // TODO: fix pad_string
    void pad_string(int padding, char* buffer, int buffer_size, char filler, int start){
        if(buffer_size == 0){ return; }
        if(buffer_size <= start){ return; }
        if(buffer_size < padding){ return; }

        int buffer_null_size = 0;
        char curScan = buffer[buffer_null_size];
        while(curScan != '\0'){
            buffer_null_size++;
            curScan = buffer[buffer_null_size];
        }
        buffer_null_size-=start;
        
        if(buffer_null_size  <= 0){ return; }
        if(padding - (buffer_null_size) <= 0){ return; }

        char tempBuffer[buffer_size];

        for(int i = 0; i < start; i++){
            tempBuffer[i] = buffer[i];
        }

        for(int i = 0; i < buffer_size; i++){
            tempBuffer[i+padding+(start-buffer_null_size)] = buffer[i+start];
        }

        for(int i = 0; i < padding-buffer_null_size; i++){
            tempBuffer[i+start] = '0';
        }

        for(int i = 0; i < buffer_size; i++){
            buffer[i] = tempBuffer[i];
        }
    }
    void setCursor(int x, int y){
        if(x > 79){ x = 79; }
        if(y > 24){ y = 24; }
        if(x < 0){ x = 0; }
        if(y < 0){ y = 0; }
        curX = x;
        curY = y;
    }
    void printf_centered(const char* toPrint, int length){
        if(length == 0){ length = calculate_string_length(toPrint); }
        int offset = (79 - length) / 2;
        setCursor(offset, curY);
        afstd::printf(toPrint);
    }
}