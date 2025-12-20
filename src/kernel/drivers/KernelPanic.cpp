#include "KernelPanic.h"
#include "Memory.h"

extern "C" void KernelPanic(InterruptRegistersError* regs){
    afstd::cls();
    SetTextColor(LRED, BLACK);
    char interrupt[22];
    afstd::printf("\n Kernel Panic!   Interrupt Code: ");
    if(regs-> int_no != 0x81){
        afstd::int_to_char_array_hex(regs->int_no, interrupt, sizeof(interrupt));
        afstd::printf(interrupt);
    }else{
        afstd::printf("Unknown");
    }
    afstd::printf("\n\n");
    SetTextColor(WHITE, BLACK);
    DrawBox(0, 0, 80, 3);
    
    uint64_t err = regs->error_code;

    bool Present = err & (1 << 0); // first bit. false = page not present
    bool RW = err & (1 << 1); // second bit. true = from write. false = from read.
    bool User = err & (1 << 2); // third bit. true = from user.
    bool Res = err & (1 << 3); // fourth bit. true = reserved field had one or more zeros.
    bool Instruction = err & (1 << 4); // fifth bit. true = was from instruction fetch in an XD page.
    bool Prot = err & (1 << 5); // protection fault.
    bool ShadowStack = err & (1 << 6); // shadow stack fault.

    char err_string[22];
    
    SetTextColor(WHITE, BLACK);
    afstd::printf("\n");
    afstd::printf("Unavailable for kernel panic.");
    DrawBox(0, 3, 46, 9, "Fault Details");
   
    char RIP_Value[22];
    afstd::int_to_char_array_hex(regs->rip, RIP_Value, sizeof(RIP_Value));
    char RSI_Value[22];
    afstd::int_to_char_array_hex(regs->rsi, RSI_Value, sizeof(RSI_Value));
    char RDI_Value[22];
    afstd::int_to_char_array_hex(regs->rdi, RDI_Value, sizeof(RDI_Value));
    char RBP_Value[22];
    afstd::int_to_char_array_hex(regs->rbp, RBP_Value, sizeof(RBP_Value));
    char CS_Value[22];
    afstd::int_to_char_array_hex(regs->cs, CS_Value, sizeof(CS_Value));
    char R8_Value[22];
    afstd::int_to_char_array_hex(regs->r8, R8_Value, sizeof(R8_Value));
    char R9_Value[22];
    afstd::int_to_char_array_hex(regs->r9, R9_Value, sizeof(R9_Value));
    char R10_Value[22];
    afstd::int_to_char_array_hex(regs->r10, R10_Value, sizeof(R10_Value));
    char R11_Value[22];
    afstd::int_to_char_array_hex(regs->r11, R11_Value, sizeof(R11_Value));
    char R12_Value[22];
    afstd::int_to_char_array_hex(regs->r12, R12_Value, sizeof(R12_Value));
    char R13_Value[22];
    afstd::int_to_char_array_hex(regs->r13, R13_Value, sizeof(R13_Value));
    char R14_Value[22];
    afstd::int_to_char_array_hex(regs->r14, R14_Value, sizeof(R14_Value));
    char R15_Value[22];
    afstd::int_to_char_array_hex(regs->r15, R15_Value, sizeof(R15_Value));
    char RAX_Value[22];
    afstd::int_to_char_array_hex(regs->rax, RAX_Value, sizeof(RAX_Value));
    char RBX_Value[22];
    afstd::int_to_char_array_hex(regs->rbx, RBX_Value, sizeof(RBX_Value));
    char RCX_Value[22];
    afstd::int_to_char_array_hex(regs->rcx, RCX_Value, sizeof(RCX_Value));
    char RDX_Value[22];
    afstd::int_to_char_array_hex(regs->rdx, RDX_Value, sizeof(RDX_Value));
    char RFLAGS_Value[22];
    afstd::int_to_char_array_binary(regs->rflags, RFLAGS_Value, sizeof(RFLAGS_Value));

    WriteString("RIP    :", 47, 4);
    WriteString(RIP_Value, 56, 4);
    WriteString("RSI    :", 47, 5);
    WriteString(RSI_Value, 56, 5);
    WriteString("RDI    :", 47, 6);
    WriteString(RDI_Value, 56, 6);
    WriteString("RBP    :", 47, 7);
    WriteString(RBP_Value, 56, 7);
    WriteString("CS     :", 47, 8);
    WriteString(CS_Value, 56, 8);
    WriteString("RAX    :", 47, 10);
    WriteString(RAX_Value, 56, 10);
    WriteString("RBX    :", 47, 11);
    WriteString(RBX_Value, 56, 11);
    WriteString("RCX    :", 47, 12);
    WriteString(RCX_Value, 56, 12);
    WriteString("RDX    :", 47, 13);
    WriteString(RDX_Value, 56, 13);
    WriteString("R8     :", 47, 14);
    WriteString(R8_Value, 56, 14);
    WriteString("R9     :", 47, 15);
    WriteString(R9_Value, 56, 15);
    WriteString("R10    :", 47, 16);
    WriteString(R10_Value, 56, 16);
    WriteString("R11    :", 47, 17);
    WriteString(R11_Value, 56, 17);
    WriteString("R12    :", 47, 18);
    WriteString(R12_Value, 56, 18);
    WriteString("R13    :", 47, 19);
    WriteString(R13_Value, 56, 19);
    WriteString("R14    :", 47, 20);
    WriteString(R14_Value, 56, 20);
    WriteString("R15    :", 47, 21);
    WriteString(R10_Value, 56, 21);
    WriteString("RFLAGS :", 47, 22);
    WriteString(RFLAGS_Value, 56, 22);

    DrawBox(46, 3, 34, 22, "System Registers");
    DrawDivider(46, 9, 34, "General Registers");

    DrawBox(0, 12, 46, 13, "Accessed Page Details");

    

    asm(
        "cli\n"
        "1:\n\t"
        "hlt\n"
        "jmp 1b\n"
        :::
    );
}