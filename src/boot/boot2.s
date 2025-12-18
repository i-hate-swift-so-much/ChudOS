[org 0x60000]
[bits 32]

_start:
    mov [kernel_arg1], eax
    mov [kernel_arg2], ebx

    ;call load_kernel

    mov byte [0xB8000], 'A'
    mov byte [0xB8001], 0x0F

    ; check if the current CPU supports the CPUID instruction
    call checkCPUID
    cmp ax, 1
    je .fail_long

    call queryLongMode
    cmp ax, 1
    je .fail_long

    jmp start_paging64

    .fail_long:
        jmp halt64

halt64:
    ;mov eax, 64
    cli
    hlt
    jmp halt64

; as of December 10th 2025 the OS is long mode compatible

; Checks if CPUID is supported by attempting to flip the ID bit (bit 21) in
; the EFLAGS register. If we can flip it, CPUID is available.
; returns eax = 1 if there is cpuid support; 0 otherwise
EFLAGS_ID equ 1 << 21           ; if this bit can be flipped, the CPUID instruction is available
CPUID_EXTENSIONS equ 0x80000000 ; returns the maximum extended requests for cpuid
CPUID_EXT_FEATURES equ 0x80000001 ; returns flags containing long mode support among other things
CPUID_EDX_EXT_FEAT_LM equ 1 << 29
checkCPUID:
    pushfd
    pop eax

    ; The original value should be saved for comparison and restoration later
    mov ecx, eax
    xor eax, EFLAGS_ID

    ; storing the eflags and then retrieving it again will show whether or not
    ; the bit could successfully be flipped
    push eax                    ; save to eflags
    popfd
    pushfd                      ; restore from eflags
    pop eax

    ; Restore EFLAGS to its original value
    push ecx
    popfd

    ; if the bit in eax was successfully flipped (eax != ecx), CPUID is supported.
    xor eax, ecx
    jnz .supported
    .notSupported:
        mov ax, 1
        ret
    .supported:
        mov ax, 0
        ret

queryLongMode:
    mov eax, CPUID_EXTENSIONS
    cpuid
    cmp eax, CPUID_EXT_FEATURES
    jb .NoLongMode              ; if the CPU can't report long mode support, then it likely doesn't have it
    
    mov eax, CPUID_EXT_FEATURES
    cpuid
    test edx, CPUID_EDX_EXT_FEAT_LM
    jz .NoLongMode
    
    mov ax, 0
    ret

    .NoLongMode:
        mov ax, 1
        ret

CODE_SEL equ gdt64_start.kernel64_code - gdt64_start
DATA_SEL equ gdt64_start.kernel64_data - gdt64_start

start_paging64:
    lgdt [gdt64_descriptor]

    mov ebx, cr0
    and ebx, ~(1 << 31)
    mov cr0, ebx

    ; Enable PAE
    mov edx, cr4
    or  edx, (1 << 5)
    mov cr4, edx

    ; Set LME (long mode enable) & NXE in the EFER register
    mov ecx, 0xC0000080
    rdmsr
    or  eax, ((1 << 8) | (1 << 11))
    wrmsr

    call fill_pt ; allocate 16 MB of ram to the kernel
    call fill_pd
    call fill_pdpt
    call fill_pml4
    
    mov eax, pml4_table
    mov ebx, pdpt_kernel
    mov ecx, pd_table_kernel
    mov edx, pt
    ;jmp halt64

    mov eax, pml4_table
    mov cr3, eax

    ; unused, kept as a reference so i know the math
    ; the below code sets the rsp to the virtual address of the top of the kernel
    ; Canonicalize      PML4 Entry      PDPT Entry      PD Entry        PT Entry      Offset
    ;(0xFFFF << 48) | (0x1FF << 39) | (0x000 << 30) | (0x000 << 21) | (0x07 << 12) | (0x0000)

    mov ebx, cr0
    or ebx, (1 << 31) | (1 << 0)
    mov cr0, ebx

    ; Now reload the segment registers (CS, DS, SS, etc.) with the appropriate segment selectors...

    mov ax, DATA_SEL
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    ; Reload CS with a 64-bit code selector by performing a long jmp

    jmp CODE_SEL:start64

fill_pml4:
    mov edi, pml4_table
    mov eax, pdpt_kernel
    mov ecx, 1
    xor edx, edx

    ; first entry is user
    push eax
    or eax, 0b00000011
    mov [edi], eax
    mov [edi+4], edx ; edx is zero
    pop eax

    ret

    .loop_code:        
        push eax
        or eax, 0b00000011
        mov [edi], eax
        mov [edi+4], edx ; edx is zero
        pop eax

        cmp ecx, 0
        je fill_pml4.end_loop

        add edi, 8
        add eax, 0x1000
        sub ecx, 1

        jmp fill_pml4.loop_code
        
    .end_loop:
        ret

fill_pdpt:
    mov edi, pdpt_kernel      ; pointer to PDPT
    mov eax, pd_table_kernel  ; lower 32 bits = PD physical address
    xor edx, edx              ; upper 32 bits = 0
    mov ecx, 1                ; 512 entries

    push eax
    or eax, 0b00000011
    mov [edi], eax
    mov [edi+4], edx ; edx is zero
    pop eax

    ret

    .fill_loop:
        cmp ecx, 0
        je fill_pdpt.end_loop

        push eax
        or eax, 0b00000011
        mov [edi], eax
        mov [edi+4], edx ; edx is zero
        pop eax

        add edi, 8
        add eax, 8

        sub ecx, 1

        jmp fill_pdpt.fill_loop

    .end_loop:
        ret

fill_pd:
    mov edi, pd_table_kernel
    mov eax, pt
    xor edx, edx
    mov ecx, 0

    ; kernel
    .kfill_loop:
        cmp ecx, 128
        je fill_pd.kloop_done

        push eax
        or eax, 0b000000011
        mov [edi], eax
        mov [edi+4], edx ; edx is zero
        pop eax

        add edi, 8
        add eax, 4096 ; next page table
        
        add ecx, 1

        jmp .kfill_loop

    .kloop_done:
        ret


fill_pt:
    mov edi, pt
    xor eax, eax ; starting address = 0
    xor edx, edx
    mov ecx, 0

    ; fills 8 page tables (16 MiB)
    .kfill_loop:
        cmp ecx, 8192
        je fill_pt.kfill_done

        push eax
        or eax, 0b100000011
        mov [edi], eax
        mov [edi+4], edx ; edx is zero
        pop eax

        add eax, 0x1000
        add edi, 8

        add ecx, 1

        jmp .kfill_loop

    .kfill_done:
        ret

; since paging exists, we don't need a base or limit for segements now
gdt64_start:
    .null_descriptor64:
        dq 0x0
    .kernel64_code:
        dw 0xFFFF ; limit
        dw 0x0 ; base
        db 0x0 ; base
        db 0b10011010 ; access byte, actually needed
        db 0b10101111 ; flags limit n shit, this just means its 64 bits
        db 0x0 ; base
    .kernel64_data:
        dw 0xFFFF ; limit
        dw 0x0 ; base
        db 0x0 ; base
        db 0b10010010 ; access byte, actually needed
        db 0b10101111 ; flags limit n shit, this just means its 64 bits
        db 0x0 ; base

gdt64_end:
gdt64_descriptor:
    dw gdt64_end - gdt64_start - 1
    dq gdt64_start

; long mode with 4 level paging 🤤
[bits 64]
start64:
    mov rsp, 0x00001000
    mov ax, DATA_SEL
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    cli

    mov rax, [kernel_arg1]
    mov rbx, [kernel_arg2]

    mov byte [0xB8002], 'B'
    mov byte [0xB8003], 0x0F

    jmp 0x80000 ; kernel

halt64_2:
    cli
    jmp halt64_2

buffer: dq 0x100000

kernel_arg1: resb 8
kernel_arg2: resb 8

align 4096
pml4_table: 
    resq 512 ; reserve 512 entries

pdpt_kernel:
    resq 512

pd_table_kernel:
    resq 512 ; blank entries

pt:
    resq 65536
