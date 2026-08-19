#pragma once
#include "stdint.h"
#include "stddef.h"
#include "stdbool.h"

#include "Devices/IO.h";

// VGA driver, started writing this to avoid AHCI.
// 1-16-26

// note to self:
// hopefully doesn't need to interact with PCI.h,
// as the controller is always mapped to the same location,
// idk though, if your monitor is burnt out it isn't my fault.

// makes use of VESA 2.0 for detection, but that code is in boot1.s

#define VGA_E_BASE_ADDRESS 0xA0000

struct VGA_DISPLAY_SETTINGS_S{
    uint8_t Mode_Control;
    uint8_t Overscan;
    uint8_t Color_Plane_Enable;

}__attribute__((packed));

// size = 256 bytes
struct VESA_ModeInfoBlock{
    uint16_t ModeAttributes;
    uint8_t FirstWindowAttributes;
    uint8_t SecondWindowAttributes;
    uint16_t WindowGranularity;
    uint16_t WindowSize;
    uint16_t FirstWindowSegment;
    uint16_t SecondWindowSegment;
    uint32_t WindowFunctionPtr;
    uint16_t BytesPerScanline;

    // revision 1.2:
    uint16_t Width;
    uint16_t Height;
    uint8_t CharWidth;
    uint8_t CharHeight;
    uint8_t PlanesCount;
    uint8_t BitsPerPixel; // sometimes VBE says this is 32 when in reality it's 24
    uint8_t BanksCount;
    uint8_t MemoryModel;
    uint8_t BankSize;
    uint8_t ImagePagesCount;
    uint8_t Reserved1;

    uint8_t RedMaskSize;
    uint8_t RedFieldPosition;
    uint8_t BlueMaskSize;
    uint8_t BlueFieldPosition;
    uint8_t GreenMaskSize;
    uint8_t GreenFieldPosition;
    uint8_t ReservedMaskSize;
    uint8_t ReservedFieldPosition;

    // revision 2.0:
    uint32_t LFBAddress;
    uint32_t OffscreenMemoryOffset;
    uint16_t OffscreeMemorySize;
}__attribute__((packed));

uint8_t VGA_Read_Index(uint8_t index);

void VGA_E_INIT(int x, int y);