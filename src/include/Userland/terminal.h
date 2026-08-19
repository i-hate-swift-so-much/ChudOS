#pragma once

#include "stddef.h"
#include "stdint.h"
#include "stdbool.h"

#include "Tasks.h"
#include "Display/VGA.h"

extern int screenOwner;

#define KERNEL_T 0
#define BASIC_T 1

struct Terminal{
    char bytes[2000]; // 80 * 25
    uint64_t lastPos;
    int w;
    int h;
    bool visible;
    uint8_t x;
    uint8_t y;
    char name[16];
    uint8_t nameLen;
    uint8_t fg;
    uint8_t bg;
};

extern struct Terminal terminals[];
void initVirtualTerminals(bool debug);
void refreshTerminals();
void virtualprint(int tid, char* src);

bool requestScreenOwnership(int pid);
void relinquishScreenOwnership(int pid);