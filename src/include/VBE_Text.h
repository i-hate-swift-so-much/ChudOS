#pragma once

#include <stdint.h>
#include "VBE.h"

struct VBE_Character{
    int PaddingTop;
    int PaddingBottom;
    int PaddingLeft;
    int PaddingRight;
    VBE_Image image;
};

namespace VBE_Text{
    void Init();
    void TypeCharacter(char Character, int x, int y);
    void ClearTextBuffer();
}