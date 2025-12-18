[org 0x1000]

; This bootloader makes use of the VBE 2.0 standard 

mov [drive_boot], dl

dap_kernel: ; read the kernel into memory
    db 0x10
    db 0x00
    dw 127
    dw 0x0000
    dw 0x8000
    dq 400

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
    mov byte [0xB8004], 'A'
    mov byte [0xB8005], 0x0F

    ; load boot3
    mov ah, 0x42
    mov dl, [drive_boot]
    mov si, dap_boot2_1
    int 0x13
    jc read_error
    mov byte [0xB8006], 'A'
    mov byte [0xB8007], 0x0F

    ; load boot3
    mov ah, 0x42
    mov dl, [drive_boot]
    mov si, dap_boot2_2
    int 0x13
    jc read_error
    mov byte [0xB8008], 'A'
    mov byte [0xB8009], 0x0F

    ;call set_VBE_mode
    mov si, boot_msg_2_1
    call print

    call get_upper_memory
    ;mov eax, [e820_cur_offset]
    ;jmp halt
    jmp swap_protected

get_upper_memory:
    ; uses bios call 0x15 eax=0xE820 to get info about the upper memory.
    mov ax, Range_Descriptor_Struct
    mov es, ax
    xor di, di
    mov ebx, 0
    mov [e820_cur_offset], 0x0

    mov dword [0x5000], 0

    .next_loop:
    xor eax, eax
    mov ax, 0xE820 ; only the lower 16 bits of eax should be set
    mov ecx, 20
    mov edx, 0x534D4150
    int 0x15
    jc get_upper_memory.e820_fail

    cmp eax, 0x534D4150
    jne get_upper_memory.e820_fail
    
    push es
    push di
    pusha

    mov ax, 0x5000
    mov es, ax
    mov di, [e820_cur_offset]

    mov ax, [Range_Descriptor_Struct.BaseAddressLow1]
    mov [es:di+4], ax
    mov ax, [Range_Descriptor_Struct.BaseAddressLow2]
    mov [es:di+6], ax
    mov ax, [Range_Descriptor_Struct.BaseAddressHigh1]
    mov [es:di+8], ax
    mov ax, [Range_Descriptor_Struct.BaseAddressHigh2]
    mov [es:di+10], ax
    mov ax, [Range_Descriptor_Struct.LengthLow1]
    mov [es:di+12], ax
    mov ax, [Range_Descriptor_Struct.LengthLow2]
    mov [es:di+14], ax
    mov ax, [Range_Descriptor_Struct.LengthHigh1]
    mov [es:di+16], ax
    mov ax, [Range_Descriptor_Struct.LengthHigh2]
    mov [es:di+18], ax
    mov ax, [Range_Descriptor_Struct.Type]
    mov [es:di+20], ax

    mov ax, di
    add ax, 24
    mov di, ax 

    mov [e820_cur_offset], di

    mov di, 0
    mov eax, [es:di]
    add eax, 1
    mov [es:di], eax

    popa
    pop di
    pop es

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
        ret

read_error:
    mov si, boot_err
    call print
    jmp halt

set_VBE_mode:
    ; Dear people reading my code, SeaBIOS only tells you a VBE mode is supported when the current framebuffer has enough space
    ; for that mode. Normally, SeaBIOS allocates 2 MB of memory for the VGA card (hasn't been industry standard since 1995).
    ; Due to this, you MUST allocate a minimum of 3 MB of memory to the VGA card when using QEMU through `-device VGA,vgamem_mb=3`


    ; tell VBE to use VBE 2.0

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

    mov ax, [VBEInfoBlock.VideoModePtr]  ; i just found out i can use eax in real mode
    mov bx, [VBEInfoBlock.VideoModePtr+2]
    ; bx = segment
    ; ax = offset
    mov es, bx
    mov si, ax

    call filter_vbe_modes

    push bx ; for preservation

    ; mask bit 14 of bx.
    ; bit 14 = use linear framebuffer
    mov ax, 0x4000 ; big number
    or bx, ax ; apply mask

    mov ax, 0x4F02 ; set VBE mode
    int 0x10
    cmp ax, 0x004F
    jne vbe_error
    mov [vbe_present], 1

    pop bx
    mov ax, bx ; for some reason i coded this as the argument
    call get_vbe_mode

    ;jmp halt

    ret
; target for filter is 1024x768x24bpp in graphical mode
filter_vbe_modes:
    mov bx, [es:si] ; set BX to the current mode
    cmp word bx, 0xFFFF ; 0xFFFF is the terminating word for the mode list
    je filter_no_match ; if we should terminate... then terminate. duh.
    ; VbeFlags is 1 byte in which each of the 4 least significant bits is a flag for deciding if the video mode is valid
    ; if the target is met, the bit is 1 and the flag is therefore set
    ; bit 0: bpp (target is 24)
    ; bit 1: resolution x (target is 1024)
    ; bit 2: resoltuion y (target is 768)
    ; bit 3: graphics mode (target is display, equivalent to bit)
    ; if [VbeFlags] is 0b00001110 then the mode is used.
    mov byte [VbeFlags], 0 ; set VbeFlags to 0

    mov ax, bx
    call get_vbe_mode ; loads info about the current mode into VBEModeInfoBlock
    add si, 2

    ; 32 bpp isn't a real thing, this is just because some VBE implementations
    ; that say 32 because of padding, in reality, 32bpp (AARRGGBB) isn't real
    ; because video memory is too basic, why would it calculate alpha?
    ; In summary, BIOSes and VBEs are very weird.
    cmp byte [VBEModeInfoBlock.BitsPerPixel], 32
    je bit_depth_y_0
    bit_depth_y_c_0:

    ; Check again, just in case.
    cmp byte [VBEModeInfoBlock.BitsPerPixel], 24
    je bit_depth_y_1
    bit_depth_y_c_1:

    cmp word [VBEModeInfoBlock.XResolution], 1024 ; offset 18 is resolution x
    je res_x_y ; if the resolution is 1024, set flag bit 1
    res_x_c: ; used by .res_x_y in place of ret

    cmp word [VBEModeInfoBlock.YResolution], 768 ; offest 20 is resolution y
    je res_y_y ; if the resolution is 768, set flag bit 2
    res_y_c:

    ; If bit 4 of VBEModeInfoBlock is enabled, then the mode is graphical.
    ; If bit 7 of VBEModeInfoBlock is enabled, then a linear framebuffer is available.
    mov cx, [VBEModeInfoBlock.ModeAttributes]
    and cx, 0x90 ; mask
    cmp cx, 0x90 
    je graphics_y ; if it's graphics and linear framebuffer set flag bit 3
    graphics_y_c:

    mov ax, [VbeFlags]
    ;and word ax, 0b0001
    mov [VbeFlags], ax
    cmp byte [VbeFlags], 0b1111 ; check flags
    je filter_done ; ensure BX is preserved

    call increment_filter_counter
    jmp filter_vbe_modes ; if flags arent correct, loop

filter_done:
    push bx
    mov si, vbe_success
    call print
    ; the below code block was used for inspecting the values with info registers on the QEMU monitor
    ; mov ax, [VBEModeInfoBlock.XResolution]
    ; mov bx, [VBEModeInfoBlock.YResolution]
    ; mov cx, [VBEModeInfoBlock.BitsPerPixel]
    ; mov dx, [VBEModeInfoBlock.PhysBasePtr]
    ; jmp halt
    pop bx
    ret

filter_no_match:
    mov ax, 0xE000
    mov si, filter_no_match_msg
    call print
    mov dx, [VbeFlags]
    mov cx, [filter_run_count]
    jmp halt
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
    mov di, VBEModeInfoBlock
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


boot_msg_2_0: db "The River Phlegethon",0x0A,0x0D,0 ; Just found out that 0x0A is newline and 0x0D is carry
boot_msg_2_1: db "Switching to protected mode...",0x0A,0x0D,0
boot_err: db "Error reading kernel from HDD.",0x0A,0x0D,0
vbe_signature: db "VBE2"
vbe_version: dw 0x0200
vbe_error_msg: db "VBE not supported.",0x0A,0x0D,0
vbe_error_msgi: db "Couldn't get VBE Info.",0x0A,0x0D,0
vbe_error_msgm: db "Couldn't get VBE Mode Info.",0x0A,0x0D,0
filter_no_match_msg: db "VBE BIOS incompatible with 1024x768x24 mode",0x0A,0x0D,0
vbe_success: db "Successfully set VBE mode",0x0A,0x0D,0
e820_fail_msg: db "Failed to obtain memory map",0x0A,0x0D,0
upper_memory_done_msg: db "Successfully obtained memory map",0x0A,0x0D,0

memory_type: resb 1
upper_memory: resq 1
set_mode: resb 2
drive_boot: resb 1

filter_run_count: resb 2

VbeFlags: resb 1

cur_row: resb 2
cur_col: resb 2

highest_ram_address: resb 4
e820_cur_offset: resb 2

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
Range_Descriptor_Struct:
    .BaseAddressLow1: resb 2
    .BaseAddressLow2: resb 2
    .BaseAddressHigh1: resb 2
    .BaseAddressHigh2: resb 2
    .LengthLow1: resb 2
    .LengthLow2: resb 2
    .LengthHigh1: resb 2
    .LengthHigh2: resb 2
    .Type: resb 4 ; 1 = usuable, other = unusable. Normally part of BIOS ROM. No clue why it has to be 4 bytes, just does.

times 2048-($-$$) db 0