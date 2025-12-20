.global isr80_stub
.global kernel_panic_stub
.global page_fault_stub
.global gpf_stub
.global invalid_opcode_stub
.global timer_interrupt_stub
.global sync_time_stub
.global keyboard_stub
.extern handle_syscall
.extern KernelPanic
.extern HandlePageFault
.extern GeneralProtectionFault
.extern TimerInterrupt
.extern InvalidOpcode
.extern SyncTime
.extern HandleKeyboardInterrupt

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

.macro check_cpl
        cmpq $0x08, 8(%rsp)
        jne 2f
        1:
                pushq %rax
                movq 16(%rsp), %rax // 16 instead of 8 due to %rax push
                movq %rax, check_priv
                popq %rax
                subq $16, %rsp // padding for dummy values
                jmp 3f
        2:
                pushq %rax
                movq 16(%rsp), %rax
                movq %rax, check_priv
                popq %rax
        3:
.endm

.macro check_cpl_ret
        cmpq $0x08, check_priv
        jne 1f
                // from kernel
                addq $16, %rsp // skip the fake dummy values
        1:
                // from user, do nothing
.endm

isr80_stub:
        check_cpl

        push_regs

        pushq $0x00

        call handle_syscall

        add $16, %rsp

        pop_regs

        check_cpl_ret

        iretq

kernel_panic_stub:
        check_cpl
        
        pushq $0x00 // error code
        
        push_regs // push general registers

        movq %cr2, %rax // push CR2
        pushq %rax // push CR2

        pushq $0x81 // interrupt number, used when kernel panic has unidentified cause

        mov %rsp, %rdi // pointer to interrupts struct
        call KernelPanic // C++ handler

        add $16, %rsp // skip the cr2 and interrupt number

        pop_regs // pop general registers

        check_cpl_ret // properly restore stack, ignore dummy values if pushed

        add $8, %rsp // skip error code

        iretq

invalid_opcode_stub:
        check_cpl

        pushq $0x00
        
        push_regs

        movq %cr2, %rax
        pushq %rax

        pushq $0x06

        mov %rsp, %rdi
        call InvalidOpcode

        add $16, %rsp

        pop_regs

        check_cpl_ret

        add $8, %rsp

        iretq

gpf_stub:
        check_cpl

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

        check_cpl_ret

        iretq

page_fault_stub:
        check_cpl

        push_regs

        movq %cr2, %rax
        pushq %rax

        pushq $0x0E

        movq %rsp, %rdi
        call HandlePageFault

        add $16, %rsp

        pop_regs

        check_cpl_ret

        iretq

timer_interrupt_stub:
        check_cpl
        
        push_regs

        movq %cr2, %rax
        pushq %rax

        pushq $0x00

        movq %rsp, %rdi
        call TimerInterrupt

        add $16, %rsp

        pop_regs

        check_cpl_ret

        iretq

sync_time_stub:
        check_cpl
        
        push_regs

        movq %cr2, %rax
        pushq %rax

        pushq $0x08

        movq %rsp, %rdi
        call SyncTime

        add $16, %rsp

        pop_regs

        check_cpl_ret

        iretq

keyboard_stub:
        check_cpl

        push_regs

        movq %cr2, %rax
        pushq %rax

        pushq $0x01
        
        movq %rsp, %rdi
        call HandleKeyboardInterrupt

        add $16, %rsp

        pop_regs

        check_cpl_ret

        iretq


.section .bss

.lcomm check_priv, 8
