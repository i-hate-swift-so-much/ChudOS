#pragma once

#include "IO.h"
#include "stdbool.h"
#include "Libraries/std.h"

enum ISA_DMA_REGISTERS{
    C1_START = 0x02,
    C1_START_UPPER = 0x83,
    C1_COUNT = 0x03,

    C2_START = 0x04,
    C2_START_UPPER = 0x81,
    C2_COUNT = 0x05,

    C3_START = 0x06,
    C3_COUNT = 0x07,
    C3_START_UPPER = 0x82,

    C5_START = 0xC4,
    C5_START_UPPER = 0x8B,
    C5_STARTCOUNT = 0xC6,

    C6_START = 0xCA,
    C6_START_UPPER = 0x89,

    STATUS = 0x08,
    COMMAND = 0x09,
    REQUEST = 0x0A,
    MODE_MASTER = 0x0B,
    MODE_SLAVE = 0xD6,
    FLIP_FLOP_MASTER = 0x0C,
    FLIP_FLOP_SLAVE = 0xD8,

    MASK_MASTER = 0x0A,
};

/**
    * @brief Sets up a DMA transfer FROM a Peripheral TO memory on any channel
    * @param Channel The channel that the peripheral is operating on.
    * @param Buffer The address (which is below the 16MB limit) that the data should be written to.
    * @param Count The amount of bytes to be transferred. Note that Count = 0 is equivalent to one byte
    * @param Auto Whether or not the transfer should automatically reset to the programm address and count after a transfer completes
    * @param Mode Which transfer mode the transfer should use. 0b00 = Transfer on Demand, 0b01 = Single, 0b10 = Block, 0b11 = Cascade
    * @return int 0 on success, int 1 means the Channel is incorrect (cannot be 0 or 4)
*/
static inline int ISA_DMA_Transfer_To(uint8_t Channel, uint32_t Buffer, uint32_t Count, bool Auto, uint8_t Mode){
    if (Channel == 0 || Channel == 4){ return 1; }

    enum ISA_DMA_REGISTERS mode_mas = MODE_MASTER; // channels < 3
    enum ISA_DMA_REGISTERS mode_slv = MODE_SLAVE; // channels > 3
    enum ISA_DMA_REGISTERS ffm = FLIP_FLOP_MASTER;
    enum ISA_DMA_REGISTERS mask_m = MASK_MASTER;

    switch (Channel){
        case 1:
            enum ISA_DMA_REGISTERS buffer_low = C1_START;
            enum ISA_DMA_REGISTERS buffer_high = C1_START_UPPER;
            enum ISA_DMA_REGISTERS count_p = C1_COUNT;
        
            outb(ffm, 0); // reset flipflop

            outb(mask_m, 0b101); // mask channel 1

            outb(ffm, 0); // reset flipflop

            outb(buffer_low, Buffer); // send out the low 16 bits of the 24 bit Buffer
            outb(buffer_low, Buffer >> 8);

            outb(ffm, 0); // reset flipflop

            outb(count_p, Count); // send out the count
            outb(count_p, Count >> 8);

            outb(buffer_high, Buffer >> 16); // send out the high 8 bits of the 24 bit Buffer
            
            outb(ffm, 0); // reset flipflop

            // then, set the mode
            uint8_t state = 0;
            state |= 0b01; // CHANNEL
            state |= 0b01 << 2;
            state |= (Auto ? 1 : 0) << 4;
            state |= Mode << 6;
            outb(mode_mas, state);
            
            outb(ffm, 0); // reset flipflop

            // unmask channel 1
            outb(mask_m, 0b01);

            break;
        case 2:
            enum ISA_DMA_REGISTERS buffer_low_c2 = C2_START;
            enum ISA_DMA_REGISTERS buffer_high_c2 = C2_START_UPPER;
            enum ISA_DMA_REGISTERS count_p_c2 = C2_COUNT;
        
            outb(ffm, 0); // reset flipflop

            outb(mask_m, 0x06); // mask channel 2

            outb(ffm, 0); // reset flipflop

            outb(buffer_low_c2, Buffer); // send out the low 16 bits of the 24 bit Buffer
            outb(buffer_low_c2, Buffer >> 8);

            outb(ffm, 0); // reset flipflop

            outb(count_p_c2, Count); // send out the count
            outb(count_p_c2, Count >> 8);

            outb(buffer_high_c2, Buffer >> 16); // send out the high 8 bits of the 24 bit Buffer
            
            outb(ffm, 0); // reset flipflop

            // then, set the mode
            uint8_t state_c2 = 0;
            state_c2 |= 0b10; // CHANNEL
            state_c2 |= 0b01 << 2;
            state_c2 |= (Auto ? 1 : 0) << 4;
            state_c2 |= Mode << 6;
            outb(mode_mas, state_c2);
            
            outb(ffm, 0); // reset flipflop

            // unmask channel 2
            outb(mask_m, 0b10);

            break;
        case 3:
            enum ISA_DMA_REGISTERS buffer_low_c3 = C3_START;
            enum ISA_DMA_REGISTERS buffer_high_c3 = C3_START_UPPER;
            enum ISA_DMA_REGISTERS count_p_c3 = C3_COUNT;
        
            outb(ffm, 0); // reset flipflop

            outb(mask_m, 0b111); // mask channel 3

            outb(ffm, 0); // reset flipflop

            outb(buffer_low_c3, Buffer); // send out the low 16 bits of the 24 bit Buffer
            outb(buffer_low_c3, Buffer >> 8);

            outb(ffm, 0); // reset flipflop

            outb(count_p_c3, Count); // send out the count
            outb(count_p_c3, Count >> 8);

            outb(buffer_high_c3, Buffer >> 16); // send out the high 8 bits of the 24 bit Buffer
            
            outb(ffm, 0); // reset flipflop

            // then, set the mode
            uint8_t state_c3 = 0;
            state_c3 |= 0b11; // CHANNEL
            state_c3 |= 0b01 << 2;
            state_c3 |= (Auto ? 1 : 0) << 4;
            state_c3 |= Mode << 6;
            outb(mode_mas, state_c3);
            
            outb(ffm, 0); // reset flipflop

            // unmask channel 3
            outb(mask_m, 0b11);

            break;
    }
    return 0;
}

/**
    * @brief ets up a DMA transfer FROM memory TO a Peripheral on any channel
    * @param Channel The channel that the peripheral is operating on.
    * @param Buffer The address (which is below the 16MB limit) that the data should be written to.
    * @param Count The amount of bytes to be transferred. Note that Count = 0 is equivalent to one byte
*/
static inline void ISA_DMA_Transfer_From(uint8_t Channel, uint32_t Buffer, uint32_t Count){

}