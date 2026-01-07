#include "syscall.h"

int curRow = 0;
int curCol = 0;

/*
int 0x80 (syscall) can only be triggered from user space because it requires the CPU to automatically push
registers RSP and SS, which are only passed in user mode. If called from kernel mode, it will result in a
garbage struct being pushed, it will push the RegistersKernelCall struct which is defined in "IDT.h" alongside
the RegistersUsersCall struct, and does not feature RSP and SS, which will corrupt the stack.
*/

void handle_syscall(InterruptRegisters regs){
    uint64_t eax_value = regs.rax;
    uint64_t ebx_value = regs.rbx;
    uint64_t ecx_value = regs.rcx;
    uint64_t edx_value = regs.rdx;

    printf("SYSCALL\n", 0);

    char buffer[22];
    int_to_char_array(regs.rax, buffer, sizeof(buffer), 0);

    printf(buffer, 0);
    printf("\n", 0);

    if(eax_value == 1){
        printf((char*)ecx_value, edx_value);
    }
}