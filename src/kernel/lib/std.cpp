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
            if(curChar == '\n'){
                curX = 0;
                curY++;
            }else if(curChar == '\0'){
                break;
            }else if(curChar == '\b'){
                curX--;
                WriteCharacter(' ', curX, curY);
            }else if(curChar == '\t'){
                curX+=8;
            }else{
                WriteCharacter(curChar, curX, curY);
                curX++;
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
    void int_to_char_array_hex(int n, char* buffer, size_t buffer_size) {
        bool negative = n < 0;
        
        n = abs(n);
        
        if (n == 0) {
            buffer[0] = '0';
            buffer[1] = 'x';
            buffer[2] = '0';
            buffer[3] = '\0';
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
        }
    }

    void int_to_char_array_binary(int n, char* buffer, size_t buffer_size){
        bool negative = n < 0;

        n = abs(n);

        if(n == 0){
            buffer[0] = '0';
            buffer[1] = 'b';
            buffer[2] = '0';
            buffer[3] = '\0';
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
        }
    }

    void pad_string(int n, char* buffer, int buffer_size, char filler, int start){
        if(buffer_size == 0){ return; }
        if(buffer_size <= start){ return; }

        int buffer_null_size = 0;
        char curScan = buffer[buffer_null_size];
        while(curScan != '\0'){
            curScan = buffer[buffer_null_size];
            buffer_null_size++;
        }

        if(buffer_size < n){ return; }
        
        char tempBuffer[buffer_size];

        for(int i = 0; i < buffer_size; i++){
            tempBuffer[i] = buffer[i];
        }

        for(int i = 0; i < buffer_null_size; i++){
            tempBuffer[i+start+n] = buffer[i+start];
        }

        for(int i = 0; i < n; i++){
            tempBuffer[i+start] = '0';
        }
    }
}