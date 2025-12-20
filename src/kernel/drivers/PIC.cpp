#include "IO.h"
#include "PIC.h"

#define PIC1 0x20
#define PIC2 0xA0
#define PIC1_COMMAND PIC1
#define PIC1_DATA (PIC1+1)
#define PIC2_COMMAND PIC2
#define PIC2_DATA (PIC2+1)

#define PIC_EOI 0x20

#define ICW1_ICW4	0x01		/* Indicates that ICW4 will be present */
#define ICW1_SINGLE	0x02		/* Single (cascade) mode */
#define ICW1_INTERVAL4	0x04		/* Call address interval 4 (8) */
#define ICW1_LEVEL	0x08		/* Level triggered (edge) mode */
#define ICW1_INIT	0x10		/* Initialization - required! */

#define ICW4_8086	0x01		/* 8086/88 (MCS-80/85) mode */
#define ICW4_AUTO	0x02		/* Auto (normal) EOI */
#define ICW4_BUF_SLAVE	0x08		/* Buffered mode/slave */
#define ICW4_BUF_MASTER	0x0C		/* Buffered mode/master */
#define ICW4_SFNM	0x10		/* Special fully nested (not) */

#define CASCADE_IRQ 2

void pic_send_eoi(uint8_t irq){
    if (irq >= 8)
        outb(PIC2_COMMAND,PIC_EOI);
    outb(PIC1_COMMAND,PIC_EOI);
}

void pic_remap(uint8_t offset1, uint8_t offset2){
    outb(PIC1_COMMAND, uint8_t(ICW1_INIT | ICW1_ICW4));  // starts the initialization sequence (in cascade mode)
	io_wait();
	outb(PIC2_COMMAND, uint8_t(ICW1_INIT | ICW1_ICW4));
	io_wait();
	outb(PIC1_DATA, offset1);                 // ICW2: Master PIC vector offset
	io_wait();
	outb(PIC2_DATA, offset2);                 // ICW2: Slave PIC vector offset
	io_wait();
	outb(PIC1_DATA, uint8_t(1 << CASCADE_IRQ));        // ICW3: tell Master PIC that there is a slave PIC at IRQ2
	io_wait();
	outb(PIC2_DATA, (uint8_t)0x02);                       // ICW3: tell Slave PIC its cascade identity (0000 0010)
	io_wait();
	
	outb(PIC1_DATA, uint8_t(ICW4_8086));               // ICW4: have the PICs use 8086 mode (and not 8080 mode)
	io_wait();
	outb(PIC2_DATA, uint8_t(ICW4_8086));
	io_wait();

    outb(PIC1_DATA, 0xFF); // mask all IRQs on master
    outb(PIC2_DATA, 0xFF); // mask all IRQs on slave
}

void pic_mask(uint8_t irq){
    uint16_t port;
    uint8_t value;

    if(irq < 8){
        port = PIC1_DATA;
    }else{
        port = PIC2_DATA;
        irq-=8;
    }
    value=inb(port) | (1 << irq);
    outb(port, value);
}

void pic_unmask(uint8_t irq){
    uint16_t port;
    uint8_t value;

    if(irq < 8){
        port = PIC1_DATA;
    }else{
        value = inb(PIC1_DATA) & ~(1 << 2);
        outb(PIC1_DATA, value);

        port = PIC2_DATA;
        irq -= 8;
    }
    value=inb(port) & ~(1 << irq);
    outb(port, value);
}