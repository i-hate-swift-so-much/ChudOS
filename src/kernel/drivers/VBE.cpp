#include "VBE.h"

uint16_t ResolutionX;
uint16_t ResolutionY;
uint64_t VRAM_ADDR;
uint8_t PIXEL_SIZE_BYTES;
uint16_t PITCH;

bool VBE_Active = false;

VBEModeInfoBlock* main_vbe_info;
VBE_Screen_Properties main_vbe_properties;

void SetMainVBE(VBEModeInfoBlock* vbe_info){
    main_vbe_info = vbe_info;
    ResolutionX = vbe_info->XResolution;
    ResolutionY = vbe_info->YResolution;
    VRAM_ADDR = (volatile uint64_t)vbe_info->PhysBasePtr;
    PIXEL_SIZE_BYTES = vbe_info->BitsPerPixel / 8;
    PITCH = vbe_info->BytesPerScanline;

    VBE_Active = true;

    main_vbe_properties.BytesPerPixel = PIXEL_SIZE_BYTES;
    main_vbe_properties.ResoltuionX = ResolutionX;
    main_vbe_properties.ResolutionY = ResolutionY;
}

bool VBEIsActive(){
    return VBE_Active;
}

void ClearScreenPixelMode(uint32_t color){
    for(int y = 0; y < ResolutionY; y++){
        for(int x = 0; x < ResolutionX; x++){
            SetPixel(x, y, color);
        }
    }
}

void RunDiagnostics(){
    uint32_t mainColor = 0xFFFFFF;

    // line diagnostics
    DrawLine(0, 0, ResolutionX, 0, mainColor);
    DrawLine(0, 0, ResolutionX, ResolutionY/4, mainColor);
    DrawLine(0, 0, ResolutionX, ResolutionY/2, mainColor);
    DrawLine(0, 0, ResolutionX, ResolutionY, mainColor);
    DrawLine(0, 0, ResolutionX/2, ResolutionY, mainColor);
    DrawLine(0, 0, ResolutionX/4, ResolutionY, mainColor);
    DrawLine(0, 0, 0, ResolutionY, mainColor);

    DrawQuad(0, 0, ResolutionX/2, 0, ResolutionX/2, ResolutionY/2, 0, ResolutionY/2, 0xFFFFFF);
    DrawSquare(0, 0, 20, 20, 0x00FF00, true);
}

void DrawSquare(int x, int y, int w, int h, uint32_t color, bool fill){
    if(fill){
        for(int curY = 0; curY < h; curY++){
            for(int curX = 0; curX < w; curX++){
                SetPixel(curX, curY, color);
            }
        }
    }else{
        DrawQuad(0, 0, w, 0, w, h, 0, h, color);
    }
}

void DrawQuad(int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3, uint32_t color){
    DrawLine(x0, y0, x1, y1, color);
    DrawLine(x1, y1, x2, y2, color);
    DrawLine(x2, y2, x3, y3, color);
    DrawLine(x3, y3, x0, y0, color);
}

void DrawLine(int x0, int y0, int x1, int y1, uint32_t color){
    /*
        Bresenham's Line Algorithm
    */

    if (x0 > x1) {
        int temp = x1;
        x1 = x0;
        x0 = temp;
    }

    if (y0 > y1) {
        int temp = y1;
        y1 = y0;
        y0 = temp;
    }

    int dx = x1-x0; // get delta x
    int dy = y1-y0; // get delta y

    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;

    int err = dx - dy;

    while (true) {
        SetPixel(x0, y0, color);

        if (x0 == x1 && y0 == y1) break;

        int e2 = 2 * err;

        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 <  dx) { err += dx; y0 += sy; }
    }
}

void SetPixel(int x, int y, uint32_t color){
    if(x < 0 || x > ResolutionX || y < 0 || y > ResolutionY) { return; }
    uint64_t base = VRAM_ADDR;
    uint64_t offset = y * PITCH + x * PIXEL_SIZE_BYTES;
    uint32_t *pixel = (uint32_t*) (offset + base);
    *pixel = color;
}

VBE_Screen_Properties GetDisplayProperties(){
    return main_vbe_properties;
}

void DrawImage(VBE_Image toDraw, int x, int y, uint32_t data){
    for(int curY = 0; curY < toDraw.height; curY++){
        for(int curX = 0; curX < toDraw.width; curX++){
            int cur_index = (curY * toDraw.width) + curX;
            uint32_t cur = toDraw.color_map[cur_index];
            SetPixel(curX+x, curY+y, toDraw.color_map[cur_index]);
        }
    }
}