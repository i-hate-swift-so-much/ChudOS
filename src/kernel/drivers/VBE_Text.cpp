#include "VBE_Text.h"

VBE_Image Logo;

int CursorX;
int CursorY;

int TextWidth;
int TextHeight;

//char TextBuffer;

namespace VBE_Text{
    void Init(){
        CursorX = 0;
        CursorY = 0;
            
        ClearTextBuffer();
    }
    void ClearTextBuffer(){
        ClearScreenPixelMode(0x000000);
    }
}

void DrawLogo(){

}