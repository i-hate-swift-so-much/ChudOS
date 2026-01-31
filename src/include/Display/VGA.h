#pragma once

#include <stdint.h>
#include <stddef.h>

#include "LowLevel/Memory.h"

#define BLACK 0x0
#define BLUE 0x1
#define GREEN 0x2
#define CYAN 0x3
#define RED 0x4
#define MAGENTA 0x5
#define BROWN 0x6
#define LGRAY 0x7
#define DGRAY 0x8
#define LBLUE 0x9
#define LGREEN 0xA
#define LCYAN 0xB
#define LRED 0xC
#define LMAGENTA 0xD
#define YELLOW 0xE
#define WHITE 0xF

extern uint8_t VGA_CUR_COLOR;

struct VGA_Queue_Character_S{
    char Character;
    uint8_t Color;
    uint8_t X;
    uint8_t Y;
}__attribute__((packed));

typedef struct VGA_Queue_Character_S VGA_Queue_Character;

void SetTextColor(uint8_t foreground, uint8_t background);
void WriteCharacter(char Character, uint8_t x, uint8_t y);
unsigned char ReadCharacter(int x, int y);
void WriteString(const char* string, int x, int y);
void DrawBox(int x, int y, int w, int h, char* title);
void DrawDivider(int x, int y, int w, char* title);
void ClearScreen();
void VGA_Scroll(int n);
size_t calculate_string_length(const char* str);
void ClearLine(int y);