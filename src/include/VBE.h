#pragma once

#include "Math.h"
#include "stdint.h"


/*
    This (coding the VBE shit) was possibly the worst experience of my life,
    this is because it took me 4 pdfs, almost switching to UEFI just so I
    didn't have to do this, relearning real mode assembly. and 3 ENTIRE DAYS 
    just to switch to VBE 2.0 1028x768x24bpp. Do you know the pain of 
    debugging assembly for 3 days straight just so you can add a single feature to
    your crappy hobby OS that will never have any meaningful impact on
    society? I'm literally just coding this because I'm bored.
*/

struct VBEModeInfoBlock {
    uint16_t ModeAttributes;
    uint8_t WinAAttributes;
    uint8_t WinBAttributes;
    uint16_t WinGranularity;
    uint16_t WinSize;
    uint16_t WinASegment;
    uint16_t WinBSegment;
    uint32_t WinFuncPtr;
    uint16_t BytesPerScanline;
    
    uint16_t XResolution;
    uint16_t YResolution;
    uint8_t XCharSize;
    uint8_t YCharSize;
    uint8_t PlaneCount;
    uint8_t BitsPerPixel;
    uint8_t BankCount;
    uint8_t MemoryModel;
    uint8_t BankSize;
    uint8_t ImagePageCount;
    uint8_t Reserved1;

    uint8_t RedMaskSize;
    uint8_t RedFieldPos;
    uint8_t GreenMaskSize;
    uint8_t GreenFieldPos;
    uint8_t BlueMaskSize;
    uint8_t BlueFieldPos;
    uint8_t ResMaskSize;
    uint8_t ResFieldPos;
    uint8_t DirectColorModeInfo;

    uint32_t PhysBasePtr;
    uint32_t OffscreenMemOffset;
    uint16_t OffscreenMemSize;
    uint8_t Reserved2[206];
} __attribute__((packed));


// This is used by drivers and user programs to get stuff like resolution
struct VBE_Screen_Properties{
    uint16_t ResoltuionX;
    uint16_t ResolutionY;
    uint8_t BytesPerPixel;
};

struct VBE_Image
{
    int width;
    int height;
    uint32_t* color_map;
};

extern uint16_t ResolutionX;
extern uint16_t ResolutionY;
extern uint64_t VRAM_ADDR;
extern uint8_t PIXEL_SIZE_BYTES;
extern uint16_t PITCH;

VBE_Screen_Properties GetDisplayProperties();
void ClearScreenPixelMode(uint32_t color);
void SetMainVBE(VBEModeInfoBlock* vbe_info);
void DrawLine(int x0, int y0, int x1, int y1, uint32_t color);
void SetPixel(int x, int y, uint32_t color);
void DrawQuad(int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3, uint32_t color);
void DrawSquare(int x, int y, int w, int h, uint32_t color, bool fill);
void DrawImage(VBE_Image, int x, int y);
void RunDiagnostics();
bool VBEIsActive();