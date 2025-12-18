#pragma once
#include <stdint.h>

void pic_remap(uint8_t offset1 = 0x20, uint8_t offset2 = 0x28);
void pic_mask(uint8_t irq);
void pic_unmask(uint8_t irq);
void pic_send_eoi(uint8_t irq);