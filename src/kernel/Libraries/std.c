#include "Libraries/std.h"
#include "Libraries/Math.h"
#include "Display/VGA.h"
#include "stdbool.h"

#define PRINTF_TYPE_NULL 0x00
#define PRINTF_TYPE_INT 0x01
#define PRINTF_TYPE_HEX 0x02
#define PRINTF_TYPE_BIN 0x03
#define PRINTF_TYPE_CHAR 0x04
#define PRINTF_TYPE_STR 0x05
#define PRINTF_TYPE_PERCENT 0x06

int curX = 0;
int curY = 0;

int snapshotX = 0;
int snapshotY = 0;

void printf(const char* toPrint, int length){
        if(length == 0 || length == NULL){ length = calculate_string_length(toPrint); }
        for (int i = 0; i < length; i++){
            if(curX > 79){ curX = 0; curY++; }
            if(curX == 79 && curY == 24) { Scroll(1); }
            char curChar = toPrint[i];
            if(curChar == ' '){
                WriteCharacter(' ', curX, curY);
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
                        NewLine();
                    }else{
                        curX = 0;
                        curY++;
                    }
                }else{
                    curX++;
                }
            }else if(curChar == '\n'){
                NewLine();
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
                    NewLine();
                    curX = 8-(79-curX);
                }else if(curX > 79-8){
                    NewLine();
                    curX = 8-(79-curX);
                }else{
                    curX+=8;
                }
            }else{
                WriteCharacter(curChar, curX, curY);
                if(curX == 79 && curY == 24){
                    NewLine();
                }else if(curX == 79){
                    NewLine();
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
void int_to_char_array(int n, char* buffer, size_t buffer_size, int padding) {
    if(padding == NULL){ padding = 0; }
    
    bool negative = n < 0;
        
    n = abs(n);
        
    if (n == 0 && padding == 0) {
        buffer[0] = '0';
        buffer[1] = '\0';
        return;
    }else if(n == 0 && padding != 0){
        buffer[0] = '0';
        buffer[1] = '\0';
        pad_string(padding, buffer, buffer_size, '0', 0);
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
            pad_string(padding, buffer, buffer_size, '0', 1);
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
            pad_string(padding, buffer, buffer_size, '0', 0);
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
            pad_string(padding, buffer, buffer_size, '0', 2);
            return;
        }

        int temp = n;
        int digits = 0;
        while (temp > 0) {
            temp /= 16;
            digits++;
        }

        size_t required_size = digits + 2 + (negative ? 1 : 0);
        if (buffer_size < required_size){
            return;
        }

        int lastI;
        // Extract digits from right to left and convert to ASCII characters
        if(negative){
            // Add null terminator at the end
            buffer[required_size+1] = '\0';

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
            pad_string(padding, buffer, buffer_size, '0', 3);
        }else{
            // Add null terminator at the end
            buffer[required_size] = '\0';

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
            pad_string(padding, buffer, buffer_size, '0', 2);
        }
    }
void int_to_char_array_binary(int n, char* buffer, size_t buffer_size, int padding){
    if(padding == NULL){ padding = 0; }
    
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
            pad_string(padding, buffer, buffer_size, '0', 2);
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
            pad_string(padding, buffer, buffer_size, '0', 3);
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
            pad_string(padding, buffer, buffer_size, '0', 2);
        }
    }
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
        printf(toPrint, length);
    }
void NewLine(){
    if(curY == 24){
        Scroll(1); 
        return; 
    }
    else{
        curX = 0;
        curY++;
    }
}
void printf_debug(const char* toPrint, int length){
    SetTextColor(LCYAN, BLACK);
    printf(toPrint, length);
    SetTextColor(WHITE, BLACK);
}
void printf_error(const char* toPrint, int length){
    SetTextColor(LRED, BLACK);
    printf(toPrint, length);
    SetTextColor(WHITE, BLACK);
}
void take_printf_snapshot(){
    snapshotX = curX;
    snapshotY = curY;
}
void printf_snapshot(const char* toPrint, int length){
    if(snapshotX == curX || snapshotY == curY){
        printf(toPrint, length);
        return;
    }
    
    int tempX = curX;
    int tempY = curY;
    curX = snapshotX;
    curY = snapshotY;
    printf(toPrint, length);
    curX = tempX;
    curY = tempY;
}
void printf_success(const char* toPrint, int length){
    SetTextColor(LGREEN, BLACK);
    printf(toPrint, length);
    SetTextColor(WHITE, BLACK);
}
void printf_debug_snapshot(const char* toPrint, int length){
    SetTextColor(LCYAN, BLACK);
    printf_snapshot(toPrint, length);
    SetTextColor(WHITE, BLACK);
}
void printf_error_snapshot(const char* toPrint, int length){
    SetTextColor(LRED, BLACK);
    printf_snapshot(toPrint, length);
    SetTextColor(WHITE, BLACK);
}
void printf_success_snapshot(const char* toPrint, int length){
    SetTextColor(LGREEN, BLACK);
    printf_snapshot(toPrint, length);
    SetTextColor(WHITE, BLACK);
}
// Gets input index of a string passed into printf, returns a PRINTF_TYPE (see lines 6-13)
int printf_get_type(const char* toScan, int length, int index){
    int cur = 0;
    int seen = 0;
    index+=1;
    while(cur < length){
        if(cur != 0 && toScan[cur - 1] == '%') 
        { seen++; }else{ cur++; continue; }

        if(seen != index){ cur++; continue; }

        switch (toScan[cur]){
            case 'i':
                return PRINTF_TYPE_INT;
                break;
            case 'd':
                return PRINTF_TYPE_INT;
                break;
            case 'x':
                return PRINTF_TYPE_HEX;
                break;
            case 'b':
                return PRINTF_TYPE_BIN;
                break;
            case 'c':
                return PRINTF_TYPE_CHAR;
                break;
            case 's':
                return PRINTF_TYPE_STR;
                break;
            case '%':
                return PRINTF_TYPE_PERCENT;
                break;
            default:
                return PRINTF_TYPE_NULL;
            break;
        }

        cur++;
    }
}
int printf_scan_length(const char* toScan, int length){
    // printf_variable is formatted just like linux's
    int ret = 0;

    while(length--){
        ret += (toScan[length-1] == '%' && toScan[length-2] != '%') ? 1 : 0;
    }

    return ret;
}
// prints starting at an index until the next argument, returns its final index
int printf_print_from(const char* toPrint, int length, int start){    
    int ret;
    
    for(int i = start; i < length; i++){
        if(toPrint[i] == '%' && toPrint[i+1] != '%'&& toPrint[i-1] != '%'){
            length = i;
            i+=2;
            ret = i;
            break;
        }
    }

    printf(toPrint+start, length-start);

    return ret;
}
int get_string_length(const char* str){
    int cur = 0;
    while(str[cur] != '\0'){ cur++; }
    return cur;
}
void printf_variable(const char* toPrint, ...){
    size_t length = get_string_length(toPrint);
    int args_length = printf_scan_length(toPrint, length);

    if(args_length == 0){printf(toPrint, 0); return;}

    va_list args;
    va_start(args, toPrint+length+1);

    int cur = 0;

    char variable_print[70];

    for(int i = 0; i < args_length; i++){
        cur = printf_print_from(toPrint, length, cur);
        int type = printf_get_type(toPrint, length, i);
        switch(type){
            case PRINTF_TYPE_NULL:
                //return;
                break;
            case PRINTF_TYPE_INT:
                int_to_char_array(va_arg(args, int), variable_print, sizeof(variable_print), 0);
                printf(variable_print, 0);
                break;
            case PRINTF_TYPE_HEX:
                int_to_char_array_hex(va_arg(args, int), variable_print, sizeof(variable_print), 0);
                printf(variable_print, 0);
                break;
            case PRINTF_TYPE_BIN:
                int_to_char_array_binary(va_arg(args, int), variable_print, sizeof(variable_print), 0);
                printf(variable_print, 0);
                break;
            default:
                break;
        }
    }
    cur = printf_print_from(toPrint, length, cur);
    va_end(args);
}
void Scroll(int n){
    curX = 0;
    snapshotY--;
    VGA_Scroll(n);
}