global _start:

[bits 64]
default rel

TSS_size equ TSS_end-TSS_start-1

section .text

mov [boot_drive], rdi

jmp _start

; sets up the TSS entry in the gdt (0x28)
make_tss_entry:
    mov rax, TSS_size
    mov [tss_limit0], ax

    lea rax, [TSS_start]
    mov [tss_base0], ax

    shr rax, 16
    mov [tss_base1], al

    mov eax, 0b00100000
    mov ebx, TSS_size
    shr ebx, 16
    and ebx, 0b1111
    or eax, ebx
    mov [tss_flags], al

    lea rax, [TSS_start]
    shr rax, 24
    mov [tss_base2], al

    lea rax, [TSS_start]
    shr rax, 32
    mov [tss_base3], eax

    ret

_start:
    extern kernel_main

    ; FUCK YES I FINALLY DID IT SUCK MY DICK IT READS THE MEMORY CORRECTLY

    mov rax, 0xffff80000009FFF0
    mov rsp, rax
    mov rbp, rax

    ; as of 20/2/26, the gdt is now set up here just because i dont want higher half shit in boot2
    call make_tss_entry

    lgdt [gdt64_descriptor]

    mov ax, 0x28
    ltr ax

    mov rdi, [boot_drive]

    call kernel_main
hang:
    mov eax, 0xFFFFFFFF
    cli
    hlt
    jmp hang


gdt64_start:
    .null_descriptor64: ; offset 0x00
        dq 0x0
    .kernel64_code: ; offset 0x08
        dw 0xFFFF ; limit
        dw 0x0 ; base
        db 0x0 ; base
        db 0b10011010 ; access byte, actually needed
        db 0b10101111 ; flags limit n shit, this just means its 64 bits
        db 0x0 ; base
    .kernel64_data: ; offset 0x10
        dw 0xFFFF ; limit
        dw 0x0 ; base
        db 0x0 ; base
        db 0b10010010 ; access byte, actually needed
        db 0b10101111 ; flags limit n shit, this just means its 64 bits
        db 0x0 ; base
    .user64_code: ; offset 0x18
        dw 0xFFFF ; limit
        dw 0x0 ; base
        db 0x0 ; base
        db 0b11111010 ; access byte, actually needed
        db 0b10101111 ; flags limit n shit, this just means its 64 bits
        db 0x0 ; base
    .user64_data: ; offset 0x20
        dw 0xFFFF ; limit
        dw 0x0 ; base
        db 0x0 ; base
        db 0b11110010 ; access byte, actually needed
        db 0b10101111 ; flags limit n shit, this just means its 64 bits
        db 0x0 ; base
    .tss_segment: ; offset 0x28
        tss_limit0: dw 0
        tss_base0: dw 0
        tss_base1: db 0
        db 0b10001001
        tss_flags: db 0
        tss_base2: db 0
        tss_base3: dd 0
        dd 0x0

gdt64_end:
gdt64_descriptor:
    dw gdt64_end - gdt64_start - 1
    dq gdt64_start

TSS_start:
    .Reserved_0:
        dd 0x00
    .RSP0:
        dq 0xffff800000FFFF00
    .RSP1:
        dq 0x00000
    .RSP2:
        dq 0x00000
    .Reserved_1:
        dq 0x00000
    .IST1:
        dq 0xffff80000003FFF0
    .IST2:
        dq 0xffff80000003EFF0
    .IST3:
        dq 0xffff80000003DFF0
    .IST4:
        dq 0xffff80000003CFF0
    .IST5:
        dq 0xffff80000003BFF0
    .IST6:
        dq 0xffff80000003AFF0
    .IST7:
        dq 0xffff800000039FF0
    .Reserved_2:
        dq 0x00000
    .Reserved_3:
        dq 0x00000
    .Reserved_4:
        dw 0x00000
    .IOPB:
        dw 0x10000
TSS_end:
boot_drive: resb 8