global _start:

section .text:
_start:
    extern kernel_main

    ; FUCK YES I FINALLY DID IT SUCK MY DICK IT READS THE MEMORY CORRECTLY

    ;mov dword [0x5000], 500
    ;mov ecx, [0x5000]

    mov rdi, rax ; is VBE available?
    mov rsi, rbx ; VBEModeInfoBlock pointer


    

    call kernel_main
    
    ;jmp hang

hang:
    mov eax, 0xFFFFFFFF
    cli
    hlt
    jmp hang