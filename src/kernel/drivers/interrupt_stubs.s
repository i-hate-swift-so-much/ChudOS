.global isr80_stub
.global kernel_panic_stub
.global page_fault_stub
.global gpf_stub
.global invalid_opcode_stub
.global timer_interrupt_stub
.extern handle_syscall
.extern KernelPanic
.extern HandlePageFault
.extern GeneralProtectionFault
.extern TimerInterrupt

isr80_stub:
        pushq %rax
        pushq %rcx
        pushq %rdx
        pushq %rbx
        pushq %rbp
        pushq %rsi
        pushq %rdi
        pushq %r8
        pushq %r9
        pushq %r10
        pushq %r11
        pushq %r12
        pushq %r13
        pushq %r14
        pushq %r15

        pushq $0x00
        pushq $0x80

        call handle_syscall

        add $16, %rsp

        popq %r15
        popq %r14
        popq %r13
        popq %r12
        popq %r11
        popq %r10
        popq %r9
        popq %r8
        popq %rdi
        popq %rsi
        popq %rbp
        popq %rbx
        popq %rdx
        popq %rcx
        popq %rax

        iretq

.macro push_regs
        pushq %rax
        pushq %rcx
        pushq %rdx
        pushq %rbx
        pushq %rbp
        pushq %rsi
        pushq %rdi
        pushq %r8
        pushq %r9
        pushq %r10
        pushq %r11
        pushq %r12
        pushq %r13
        pushq %r14
        pushq %r15
.endm

.macro pop_regs
        popq %r15
        popq %r14
        popq %r13
        popq %r12
        popq %r11
        popq %r10
        popq %r9
        popq %r8
        popq %rdi
        popq %rsi
        popq %rbp
        popq %rbx
        popq %rdx
        popq %rcx
        popq %rax
.endm

kernel_panic_stub:
        push_regs

        movq %cr2, %rax
        pushq %rax

        pushq $0x81

        mov %rsp, %rdi
        call KernelPanic

        add $16, %rsp

        pop_regs

        iretq

invalid_opcode_stub:
        pushq $0x00
        
        push_regs

        movq %cr2, %rax
        pushq %rax

        pushq $0x06

        mov %rsp, %rdi
        call KernelPanic

        add $16, %rsp

        pop_regs

        iretq

gpf_stub:
        push_regs

        movq %cr2, %rax
        pushq %rax

        pushq $0x0D

        mov %rsp, %rdi
        call GeneralProtectionFault

        add $16, %rsp

        popq %rax
        movq %rax, %cr2

        pop_regs

        iretq

page_fault_stub:
        pushq %rax
        pushq %rcx
        pushq %rdx
        pushq %rbx
        pushq %rbp
        pushq %rsi
        pushq %rdi
        pushq %r8
        pushq %r9
        pushq %r10
        pushq %r11
        pushq %r12
        pushq %r13
        pushq %r14
        pushq %r15

        movq %cr2, %rax
        pushq %rax

        pushq $0x0E

        movq %rsp, %rdi
        call HandlePageFault

        add $16, %rsp

        popq %rax
        movq %rax, %cr2

        popq %r15
        popq %r14
        popq %r13
        popq %r12
        popq %r11
        popq %r10
        popq %r9
        popq %r8
        popq %rdi
        popq %rsi
        popq %rbp
        popq %rbx
        popq %rdx
        popq %rcx
        popq %rax

        iretq

timer_interrupt_stub:
        push_regs

        pushq $0x00
        pushq $0x00

        movq %rsp, %rdi
        call TimerInterrupt

        add $16, %rsp

        pop_regs

        iretq

.section .bss

.lcomm check_priv, 8
