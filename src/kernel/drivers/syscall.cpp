#include "syscall.h"

int curRow = 0;
int curCol = 0;

/*
int 0x80 (syscall) can only be triggered from user space because it requires the CPU to automatically push
registers RSP and SS, which are only passed in user mode. If called from kernel mode, it will result in a
garbage struct being pushed, it will push the RegistersKernelCall struct which is defined in "IDT.h" alongside
the RegistersUsersCall struct, and does not feature RSP and SS, which will corrupt the stack.
*/

extern "C" void handle_syscall(RegistersUsersCall regs){
    uint64_t eax_value = regs.rax;
    uint64_t ebx_value = regs.rbx;
    uint64_t ecx_value = regs.rcx;
    uint64_t edx_value = regs.rdx;

    afstd::printf("SYSCALL\n");

    char buffer[22];
    afstd::int_to_char_array(regs.rax, buffer, sizeof(buffer));

    afstd::printf(buffer);
    afstd::printf("\n");

    if(eax_value == 1){
        afstd::printf((char*)ecx_value, edx_value);
    }
}