#include "Userland/syscall.h"

int curRow = 0;
int curCol = 0;

#define STD_IN_FD 0
#define STD_OUT_FD 1

/*
int 0x80 (syscall) can only be triggered from user space because it requires the CPU to automatically push
registers RSP and SS, which are only passed in user mode. If called from kernel mode, it will result in a
garbage struct being pushed, it will push the RegistersKernelCall struct which is defined in "IDT.h" alongside
the RegistersUsersCall struct, and does not feature RSP and SS, which will corrupt the stack.
*/

int lastPrintX = 0;
int lastPrintY = 0;

void handle_syscall(InterruptRegisters regs){
    uint64_t rax_value = regs.rax;
    uint64_t rbx_value = regs.rbx;
    uint64_t rcx_value = regs.rcx;
    uint64_t rdx_value = regs.rdx;

    switch (rax_value){
        case 1:
            // WRITE
            // rdx = descriptor
            // rcx = byte count
            // rbx = buffer address
            if(rdx_value == STD_OUT_FD){
                char* msg = (char*)rbx_value;
                for(int i = 0; i < rcx_value; i++){
                    WriteCharacter(msg[i], i, 0);
                    lastPrintX++;
                }
            }
            break;
    }
}