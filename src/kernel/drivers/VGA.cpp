#include "VGA.h"

#define VGA_WIDTH 80
#define VGA_HEIGHT 25

uint8_t* vga_buffer = (uint8_t*)0xB8000;

uint8_t VGA_CUR_COLOR = 0x0F;

const uint16_t VGA_ADDRESS = 0xB8000;

size_t calculate_string_length(const char* str) {
    size_t length = 0;
    while (str[length] != '\0') {
        length++;
    }
    return length;
}

void SetTextColor(uint8_t foreground, uint8_t background){
    VGA_CUR_COLOR = (background << 4) | foreground;
}

void WriteCharacter(char Character, int x, int y){
    size_t offset = (y * 80 + x) * 2;
    char* ptr = (char*)(0xB8000+offset);
    ptr[0] = Character;
    ptr[1] = VGA_CUR_COLOR;
}

void DrawBox(int x, int y, int w, int h, char* title){
    bool t = true;

    int title_length;

    uint8_t init_cur_color = VGA_CUR_COLOR;

    title_length = calculate_string_length(title);

    if (title_length <= 0){
        t = false;
    }
    
    char TOP_LEFT = 0xC9;
    char TOP_RIGHT = 0xBB;
    char BOT_LEFT = 0xC8;
    char BOT_RIGHT = 0xBC;
    char HOR_LINE = 0xCD;
    char VER_LINE = 0xBA;
    // Draw the box outline
    for(int curY = 0; curY < h; curY++){
        for(int curX = 0; curX < w; curX++){
            if(curX == 0 && curY == 0){ WriteCharacter(TOP_LEFT, curX+x, curY+y); continue; }
            if(curX == w-1 && curY == 0){ WriteCharacter(TOP_RIGHT, curX+x, curY+y); continue; }
            if(curX == 0 && curY == h-1){ WriteCharacter(BOT_LEFT, curX+x, curY+y); continue; }
            if(curX == w-1 && curY == h-1){ WriteCharacter(BOT_RIGHT, curX+x, curY+y); continue; }
            if(curX == 0 || curX == w-1){ WriteCharacter(VER_LINE, curX+x, curY+y); continue; }
            if(curY == 0 || curY == h-1){ WriteCharacter(HOR_LINE, curX+x, curY+y); continue; }
        }
    }
    // Write the title
    if(t){
        int leftPadding = (w - title_length) / 2;
        if(leftPadding < 0){ leftPadding = 0; }
        int titleX = x+leftPadding;
        WriteString(title, titleX, y);
        for(int i = 0; i < title_length; i++){
            VGA_CUR_COLOR = ~init_cur_color;
            WriteCharacter(title[i], i+titleX, y);
        }
    }

    VGA_CUR_COLOR = init_cur_color;
}

void DrawDivider(int x, int y, int w, char* title){
    unsigned char TOP_LEFT = 0xC9;
    unsigned char TOP_RIGHT = 0xBB;
    unsigned char BOT_LEFT = 0xC8;
    unsigned char BOT_RIGHT = 0xBC;
    unsigned char HOR_LINE = 0xCD;
    unsigned char VER_LINE = 0xBA;

    uint8_t init_cur_color = VGA_CUR_COLOR;

    bool t = true;

    int title_length = calculate_string_length(title);

    if(title_length == 0){ t = false; }

    for(int cx = 0; cx < w; cx++){
        unsigned char above = ReadCharacter(cx+x, y-1);
        unsigned char below = ReadCharacter(cx+x, y+1);

        if(cx == 0){
            if(above == VER_LINE && below == VER_LINE){
                WriteCharacter(0xCC, cx+x, y);
            }else if(above == VER_LINE){
                WriteCharacter(BOT_LEFT, cx+x, y);
            }else if(below == VER_LINE){
                WriteCharacter(TOP_LEFT, cx+x, y);
            }else{
                WriteCharacter(0xCD, cx+x, y);
            }
        }else if(cx == w-1){
            if(above == VER_LINE && below == VER_LINE){
                WriteCharacter(0xB9, cx+x, y);
            }else if(above == VER_LINE){
                WriteCharacter(BOT_LEFT, cx+x, y);
            }else if(below == VER_LINE){
                WriteCharacter(TOP_LEFT, cx+x, y);
            }else{
                WriteCharacter(0xCD, cx+x, y);
            }
        }else{
            WriteCharacter(0xCD, cx+x, y);
        }
    }
    
    if(t){
        int leftPadding = (w - title_length) / 2;
        if(leftPadding < 0){ leftPadding = 0; }
        int titleX = x+leftPadding;
        WriteString(title, titleX, y);
        for(int i = 0; i < title_length; i++){
            VGA_CUR_COLOR = ~init_cur_color;
            WriteCharacter(title[i], i+titleX, y);
        }
    }
    VGA_CUR_COLOR = init_cur_color;
}

void WriteLabel(char* label, char* value, int x, int y, int w){
    size_t length = calculate_string_length(label)-1;
    // TODO: make this into a function that prints one side of a label
    // on the left half of this, then prints the value on the right half
    // as seen in the exception screens
}

void WriteString(const char* string, int x, int y){
    size_t length = calculate_string_length(string);
    for(int i = 0; i < length; i++){
        WriteCharacter(string[i], x+i, y);
    }
}

void ClearScreen(){
    for(int i = 0; i < 80*25; i++){
        WriteCharacter(' ', i, 0);
    }
}

// when using this, it should ALWAYS be unsigned.
// e.g. `unsigned char = ReadCharacter(0, 0);` is valid,
// but `char = ReadCharacter(0, 0);` is invalid.
// If you read it as signed, it will become negative.
unsigned char ReadCharacter(int x, int y){
    size_t offset = (y * 80 + x) * 2;
    unsigned char* ptr = (unsigned char*)(0xB8000+offset);
    return ptr[0];
};