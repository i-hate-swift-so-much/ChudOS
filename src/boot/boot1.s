[org 0x1000]

mov [drive_boot], dl

dap_kernel: ; read the kernel into memory
    db 0x10
    db 0x00
    dw 127
    dw 0x0000
    dw 0x8000
    dq 400

dap_kernel_setup: ; read the kernel into memory
    db 0x10
    db 0x00
    dw 127
    dw 0x0000
    dw 0x4000
    dq 600

dap_boot2_1: ; read the first half of the third stage into memory
    db 0x10
    db 0x00
    dw 127
    dw 0x0000
    dw 0x6000
    dq 6

dap_boot2_2: ; read the second half of the third stage into memory
    db 0x10
    db 0x00
    dw 127
    dw 0xFE00
    dw 0x6000
    dq 133

; Reset segment registers (again)
cli
mov ax, cs
mov ds, ax
mov es, ax

xor ax, ax
mov ss, ax
mov sp, 0x1FFF
sti

mov si, boot_msg_2_0
call clear_screen
call print

code_segment equ kernel_code_descriptor - gdt_start
data_segment equ kernel_data_descriptor - gdt_start

load_stage3:
    ; load kernel
    mov ah, 0x42
    mov dl, [drive_boot]
    mov si, dap_kernel
    int 0x13
    jc read_error

    ; load kernel setup
    mov ah, 0x42
    mov dl, [drive_boot]
    mov si, dap_kernel_setup
    int 0x13
    jc read_error

    ; load boot3
    mov ah, 0x42
    mov dl, [drive_boot]
    mov si, dap_boot2_1
    int 0x13
    jc read_error

    ; load boot3
    mov ah, 0x42
    mov dl, [drive_boot]
    mov si, dap_boot2_2
    int 0x13
    jc read_error

    ;call set_VBE_mode
    mov si, boot_msg_2_1
    call print

    call get_upper_memory
    ;mov eax, [e820_cur_offset]
    ;jmp halt
    jmp swap_protected

get_upper_memory:
    ; uses bios call 0x15 eax=0xE820 to get info about the upper memory.
    mov [e820_cur_offset], 0

    xor ebx, ebx
    .next_loop:
    mov ax, 0xF00
    mov es, ax ; set ES to 0xF000, which is used as the base address of the list of memory
    mov edi, [e820_cur_offset]
    xor eax, eax
    mov ax, 0xE820 ; only the lower 16 bits of eax should be set
    mov ecx, 24
    mov edx, 0x534D4150
    int 0x15
    jc get_upper_memory.e820_fail

    pusha

    cmp eax, 0x534D4150
    jne get_upper_memory.e820_fail

    mov eax, [e820_cur_offset]
    add eax, 24
    mov [e820_cur_offset], eax

    popa

    cmp ebx, 0
    je get_upper_memory.upper_memory_done

    jmp get_upper_memory.next_loop

    .e820_fail:
        mov si, e820_fail_msg
        call print
        jmp halt

    .upper_memory_done:  
        mov si, upper_memory_done_msg
        call print

        mov eax, [e820_cur_offset]
        mov [0xEFE8], eax

        ret

read_error:
    mov si, boot_err
    call print
    jmp halt




make_vbe_array:
    ; Dear people reading my code, SeaBIOS only tells you a VBE mode is supported when the current framebuffer has enough space
    ; for that mode. Normally, SeaBIOS allocates 2 MB of memory for the VGA card (hasn't been industry standard since 1995).
    ; Due to this, you MUST allocate a minimum of 3 MB of memory to the VGA card when using QEMU through `-device VGA,vgamem_mb=3`


    ; tell VBE to use VBE 2.0

    mov [vbe_cur_offset], 0

    mov ax, [vbe_signature] ; 'VBE2'
    mov [VBEInfoBlock.VbeSignature], ax
    mov ax, [vbe_version] ; '0x0200'
    mov [VBEInfoBlock.VbeVersion], ax ; upper byte is major, lower byte is minor

    mov ax, cs
    mov es, ax
    mov di, VBEInfoBlock
    mov ax, 0x4F00 ; get VBE info
    int 0x10
    cmp ax, 0x004F
    jne vbe_errori

    mov ax, [VBEInfoBlock.VideoModePtr]
    mov bx, [VBEInfoBlock.VideoModePtr+2]
    ; bx = segment
    ; ax = offset
    mov es, bx
    mov si, ax

    pop bx
    mov ax, bx ; for some reason i coded this as the argument
    mov bx, [vbe_cur_offset]
    call get_vbe_mode

    mov bx, [vbe_cur_offset]
    add bx, 

    ;jmp halt

    ret
flag_all_debug:
    mov ax, [VbeFlags]
    or ax, 0b1111
    mov [VbeFlags], ax
    ret
bit_depth_y_0:
    mov ax, [VbeFlags]
    or ax, 0b0001
    mov [VbeFlags], ax ; sets the least significant bit of the flags to 1
    jmp bit_depth_y_c_0
bit_depth_y_1:
    mov ax, [VbeFlags]
    or ax, 0b0001
    mov [VbeFlags], ax ; sets the least significant bit of the flags to 1
    jmp bit_depth_y_c_1
res_x_y:
    mov ax, [VbeFlags]
    or ax, 0b0010
    mov [VbeFlags], ax
    jmp res_x_c
res_y_y:
    mov ax, [VbeFlags]
    or ax, 0b0100
    mov [VbeFlags], ax
    jmp res_y_c
graphics_y:
    mov ax, [VbeFlags]
    or ax, 0b1000
    mov [VbeFlags], ax
    jmp graphics_y_c

get_vbe_mode:
    push si
    push es
    push di
    mov cx, ax ; set cx to the cur mode
    mov ax, cs
    mov es, ax
    mov di, bx
    mov ax, 0x4F01
    int 0x10
    pop di
    pop es
    pop si
    cmp ax, 0x004F
    jne vbe_errorm
    ret

vbe_errori:
    mov si, vbe_error_msgi
    call print
    mov ax, 0xE001
    mov dx, [VbeFlags]
    mov cx, [filter_run_count]
    jmp halt

vbe_errorm:
    mov si, vbe_error_msgm
    call print
    mov ax, 0xE002
    mov dx, [VbeFlags]
    mov cx, [filter_run_count]
    jmp halt

vbe_error:
    mov ax, 0xE003
    mov dx, [VbeFlags]
    mov cx, [filter_run_count]
    jmp halt

increment_filter_counter:
    mov ax, [filter_run_count]
    add ax, 1
    mov [filter_run_count], ax
    ret

swap_protected:
    cli ; NO BIOS ANYMORE NOOOO
    lgdt [gdt_descriptor]
    mov eax, cr0 ; set least significant bit of cr0 to 1
    or eax, 1
    mov cr0, eax ; oh my god finally this took so long
    ; far jump to finish up the switch

    jmp code_segment:start_protected

clear_screen:
    mov ah, 0x06
    mov al, 0x00
    mov bh, 0x07
    mov ch, 0x00
    mov cl, 0x00
    mov dh, 24
    mov dl, 79
    int 0x10
    mov ah, 0x02
    mov bh, 0
    mov dh, 0
    mov dl, 0
    int 0x10
    ret

print:
    lodsb ; load byte at ds:si to AL and increments SI
    cmp al, 0 ; check for null terminator
    je .done
    mov ah, 0x0E
    int 0x10
    jmp print
.done:
    ret



halt:
    hlt
    jmp halt

gdt_start:
    null_descriptor:
        dd 0
        dd 0
    kernel:
        kernel_code_descriptor:
            dw 0xffff ; limit low
            dw 0x0 ; base lower word
            db 0x0 ; base middle byte
            db 0b10011010 ; access byte: P, DPL (2b), S, E, DC, RW, A
            db 0b11001111 ; flags: G, D, L, R. limit high = F
            db 0x0 ; base upper byte
        kernel_data_descriptor:
            dw 0xffff ; limit
            dw 0x0 ; base lower word
            db 0x0 ; base middle byte
            db 0b10010010 ; access byte: P, DPL (2b), S, E, DC, RW, A
            db 0b11001111 ; flags: G, D, L, R. limit high
            db 0x0 ; base upper byte
gdt_end:
gdt_descriptor:
    dw gdt_end - gdt_start - 1 ; size
    dd gdt_start ; pointer


[bits 32]
start_protected:
    push ds ; save data segment for later pointer shit
    mov ax, data_segment
    mov ds, ax
	mov ss, ax
	mov es, ax
	mov fs, ax
	mov gs, ax

    ; set stack to begin right before the kernel starts
    mov ebp, 0x1FFF
	mov esp, ebp

    ; convert the segment:offset of the VBEModeInfoBlock into a 32 bit linear address
    ; cus i just found out passing a label is the offset not the address
    ; fuck my stupid chungus life

    ; linear address = (segment * 0x10) + offset
    pop eax ; segment
    mov ebx, VBEModeInfoBlock ; offset
    imul eax, 0x10 ; segment * 0x10
    add eax, ebx ; segment + offset

    mov ebx, [vbe_present]
    ;jmp halt32

    jmp 0x60000 ; go to boot2


    

halt32:
    hlt
    jmp halt32


boot_msg_2_0: db "BOOT_1",0x0A,0x0D,0 ; Just found out that 0x0A is newline and 0x0D is carry
boot_msg_2_1: db "PROT_1",0x0A,0x0D,0
boot_err: db "READ_ERR",0x0A,0x0D,0
vbe_signature: db "VBE2"
vbe_version: dw 0x0200
vbe_error_msg: db "VBE not supported.",0x0A,0x0D,0
vbe_error_msgi: db "Couldn't get VBE Info.",0x0A,0x0D,0
vbe_error_msgm: db "Couldn't get VBE Mode Info.",0x0A,0x0D,0
filter_no_match_msg: db "VBE BIOS incompatible with 1024x768x24 mode",0x0A,0x0D,0
vbe_success: db "Successfully set VBE mode",0x0A,0x0D,0
e820_fail_msg: db "MEM_MAP_N",0x0A,0x0D,0
upper_memory_done_msg: db "MEM_MAP_Y",0x0A,0x0D,0

memory_type: resb 1
upper_memory: resq 1
set_mode: resb 2
drive_boot: resb 1

filter_run_count: resb 2

VbeFlags: resb 1

cur_row: resb 2
cur_col: resb 2

e820_cur_offset: resb 4

vbe_cur_offset: resb 4

vbe_present: resb 1

VBEInfoBlock:
    .VbeSignature: resb 4 ; VBE2
    .VbeVersion: resw 1 ; 0x0200
    .OemStringPtr: resd 1
    .Capabilities: resb 4
    .VideoModePtr: resd 1
    .TotalMemory: resw 1

    .OemSoftwareRev: resw 1
    .OemVendorNamePtr: resd 1
    .OemProductNamePtr: resd 1
    .OemProductRevPtr: resd 1
    .Reserved1: resb 222

    .OemData: resb 256

VBEModeInfoBlock:
    .ModeAttributes: resw 1
    .WinAAttributes: resb 1
    .WinBAttributes: resb 1
    .WinGranularity: resw 1
    .WinSize: resw 1
    .WinASegment: resw 1
    .WinBSegment: resw 1
    .WinFuncPtr: resd 1
    .BytesPerScanline: resw 1
    
    .XResolution: resw 1
    .YResolution: resw 1
    .XCharSize: resb 1
    .YCharSize: resb 1
    .PlaneCount: resb 1
    .BitsPerPixel: resb 1
    .BankCount: resb 1
    .MemoryModel: resb 1
    .BankSize: resb 1
    .ImagePageCount: resb 1
    .Reserved1: resb 1

    .RedMaskSize: resb 1
    .RedFieldPos: resb 1
    .GreenMaskSize: resb 1
    .GreenFieldPos: resb 1
    .BlueMaskSize: resb 1
    .BlueFieldPos: resb 1
    .ResMaskSize: resb 1
    .ResFieldPos: resb 1
    .DirectColorModeInfo: resb 1

    .PhysBasePtr: resd 1
    .OffscreenMemOffset: resd 1
    .OffscreenMemSize: resw 1
    .Reserved2: resb 206


times 2048-($-$$) db 0