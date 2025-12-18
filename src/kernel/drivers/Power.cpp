#include "Power.h"

extern "C" void reboot_triple_fault() {
    // Create a zeroed IDTR (limit=0, base=0)
    struct { uint16_t limit; uint64_t base; } __attribute__((packed)) null_idtr = { 0, 0 };
    asm volatile (
        "lidt %0\n"        // load zero IDT
        "int $3\n"         // trigger breakpoint exception -> will triple fault
        :
        : "m"(null_idtr)
        : "memory"
    );
    // If it returns, fall back to other methods
}

extern "C" void reboot_cf9() {
    unsigned char cf9 = inb(0xCF9) & ~0x0E; // clear relevant bits
    outb(0xCF9, cf9 | 0x02);
    // small delay (simple I/O or loop)
    for (volatile int i = 0; i < 100000; ++i) asm volatile("nop");
    outb(0xCF9, cf9 | 0x06);  // set SYS_RST (edge) to actually reset
}