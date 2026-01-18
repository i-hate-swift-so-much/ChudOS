global _start:

[bits 64]
section .text:
_start:
    extern kernel_setup_main

    mov rax, 0x3FFF0
    mov rsp, rax
    mov rbp, rax

    call kernel_setup_main
hang:
    mov eax, 0xFFFFFFFA
    cli
    hlt
    jmp hang