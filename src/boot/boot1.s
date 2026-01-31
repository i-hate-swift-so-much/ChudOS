[org 0x1000]

mov [drive_boot], dl

jmp segments

align 16
dap_kernel: ; read the kernel into memory
    db 0x10
    db 0x00
    dw 127
    dw 0x0000
    dw 0x8000
    dq 400

align 16
dap_kernel_setup: ; read the kernel into memory
    db 0x10
    db 0x00
    dw 1
    dw 0x0000
    dw 0x4000
    dq 600

align 16
dap_boot2_1: ; read the first half of the third stage into memory
    db 0x10
    db 0x00
    dw 127
    dw 0x0000
    dw 0x6000
    dq 20

align 16
dap_boot2_2: ; read the second half of the third stage into memory
    db 0x10
    db 0x00
    dw 127
    dw 0xFE00
    dw 0x6000
    dq 147

segments:
; Reset segment registers (again)
cli
xor ax, ax
mov ss, ax
mov sp, 0x7000 ; set stack pointer
mov ds, ax
mov es, ax
sti ; re-enable interrupts

call clear_screen

mov al, '2'
mov ah, 0x0E
int 0x10

mov dl, [drive_boot]
cmp dl, 0x80
je load_stage3
jmp load_stage3_floppy

code_segment equ kernel_code_descriptor - gdt_start
data_segment equ kernel_data_descriptor - gdt_start

load_stage3:
    mov al, 'H'
    mov ah, 0x0E
    int 0x10

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


    call get_upper_memory
    jmp swap_protected

load_stage3_floppy:
    mov al, 'F'
    mov ah, 0x0E
    int 0x10
    
    call get_floppy_info
    
    call load_kernel_floppy
    call load_kernel_setup_floppy
    call load_boot2_1_floppy

    call get_upper_memory
    ;jmp halt
    jmp swap_protected

; set AX to the desired LBA, then the target C H and S will be set accordingly. must call get_floppy_info first
calculate_LBA_to_CHS:
    pusha
    xor bx, bx
    xor dx, dx
    movzx bx, byte [FloppyInfoStruct.sector_max]
    div bx

    inc dx
    mov [target_sector], dl

    xor bx, bx
    xor dx, dx
    mov bl, [FloppyInfoStruct.head_count]

    div bx

    mov [target_head], dl
    mov [target_cylinder], al

    popa

    ret

get_floppy_info:
    pusha

    ; clear es:di
    xor ax, ax
    mov es, ax
    xor di, di

    mov ah, 0x08
    mov dl, [drive_boot]
    int 0x13

    mov [FloppyInfoStruct.drive_count], dl

    mov dl, dh
    mov dh, 0

    mov [FloppyInfoStruct.head_max], dl
    inc dl
    mov [FloppyInfoStruct.head_count], dl

    mov al, cl
    and al, 0x3F
    mov [FloppyInfoStruct.sector_max], al

    mov ax, cx
    xchg al, ah
    shr ah, 6

    mov word [FloppyInfoStruct.cylinder_max], ax

    popa

    ret

load_kernel_setup_floppy:
    ; starting LBA = 600
    mov ax, 600
    call calculate_LBA_to_CHS

    mov dword [floppy_target_address], 0x40000

    mov [floppy_read_loop], 1
    .loop:
    mov al, [floppy_read_loop]
    cmp al, 0
    je load_boot2_1_floppy.loop_success

    mov si, 3 ; for each sector read, we can retry 3 times
    .retry:

    mov al, [target_sector]
    cmp al, [FloppyInfoStruct.sector_max]
    jg load_boot2_1_floppy.over_sector
    jmp load_boot2_1_floppy.valid_sector

    .over_sector:
        ; correct the sector count
        mov al, [target_sector]
        sub al, [FloppyInfoStruct.sector_max]
        mov [target_sector], al

        mov al, [target_head]
        inc al
        mov [target_head], al

        cmp al, [FloppyInfoStruct.head_max]
        jg load_boot2_1_floppy.over_head
        jmp load_boot2_1_floppy.valid_sector

    .over_head:
        mov [target_head], 0

        mov al, [target_cylinder]
        inc al
        mov [target_cylinder], al

    .valid_sector:

    xor ax, ax
    mov ds, ax
    mov ch, [target_cylinder] ; cylinder
    mov cl, [target_sector] ; sector
    and cl, 0x3F
    mov al, [target_cylinder]
    shr al, 2
    and al, 0xC0
    or cl, al
    mov dh, [target_head] ; head
    mov dl, [drive_boot]
    mov dword eax, [floppy_target_address]
    shr dword eax, 4
    mov es, ax
    xor bx, bx
    mov ah, 0x2
    mov al, 1 ; sectors to read
    int 0x13
    jnc load_boot2_1_floppy.floppy_success

    dec si
    jnz load_boot2_1_floppy.retry
    jmp fail_floppy

    .floppy_success:
        mov al, [target_sector]
        add al, 1
        mov [target_sector], al

        mov al, [floppy_read_loop]
        dec al
        mov [floppy_read_loop], al

        mov dword eax, [floppy_target_address]
        add eax, 512
        mov dword [floppy_target_address], eax

        jmp load_boot2_1_floppy.loop
    .loop_success:
        mov al, '1'
        mov ah, 0x0E
        int 0x10

        ret

load_kernel_floppy:
    ; LBA = 400
    mov ax, 400
    call calculate_LBA_to_CHS

    mov dword [floppy_target_address], 0x80000

    mov [floppy_read_loop], 127
    .loop:
    mov al, [floppy_read_loop]
    cmp al, 0
    je load_kernel_floppy.loop_success

    mov si, 3 ; for each sector read, we can retry 3 times
    .retry:

    mov al, [target_sector]
    cmp al, [FloppyInfoStruct.sector_max]
    jg load_kernel_floppy.over_sector
    jmp load_kernel_floppy.valid_sector

    .over_sector:
        ; correct the sector count
        mov al, [target_sector]
        sub al, [FloppyInfoStruct.sector_max]
        mov [target_sector], al

        mov al, [target_head]
        inc al
        mov [target_head], al

        cmp al, [FloppyInfoStruct.head_max]
        jg load_kernel_floppy.over_head
        jmp load_kernel_floppy.valid_sector

    .over_head:
        mov [target_head], 0

        mov al, [target_cylinder]
        inc al
        mov [target_cylinder], al

    .valid_sector:

    xor ax, ax
    mov ds, ax
    mov ch, [target_cylinder] ; cylinder
    mov cl, [target_sector] ; sector
    and cl, 0x3F
    mov al, [target_cylinder]
    shr al, 2
    and al, 0xC0
    or cl, al
    mov dh, [target_head] ; head
    mov dl, [drive_boot]
    mov dword eax, [floppy_target_address]
    shr dword eax, 4
    mov es, ax
    xor bx, bx
    mov ah, 0x2
    mov al, 1 ; sectors to read
    int 0x13
    jnc load_kernel_floppy.floppy_success

    dec si
    jnz load_kernel_floppy.retry
    jmp fail_floppy

    .floppy_success:
        mov al, [target_sector]
        add al, 1
        mov [target_sector], al

        mov al, [floppy_read_loop]
        sub al, 1
        mov [floppy_read_loop], al

        mov dword eax, [floppy_target_address]
        add eax, 512
        mov dword [floppy_target_address], eax

        jmp load_kernel_floppy.loop
    .loop_success:
        mov al, '1'
        mov ah, 0x0E
        int 0x10
        ret

load_boot2_1_floppy:
    ; LBA = 20
    mov ax, 20
    call calculate_LBA_to_CHS

    mov dword [floppy_target_address], 0x60000

    mov [floppy_read_loop], 254

    .loop:
    mov al, [floppy_read_loop]
    cmp al, 0
    je load_boot2_1_floppy.loop_success

    mov si, 3 ; for each sector read, we can retry 3 times
    .retry:

    mov al, [target_sector]
    cmp al, [FloppyInfoStruct.sector_max]
    jg load_boot2_1_floppy.over_sector
    jmp load_boot2_1_floppy.valid_sector

    .over_sector:
        ; correct the sector count
        mov al, [target_sector]
        sub al, [FloppyInfoStruct.sector_max]
        mov [target_sector], al

        mov al, [target_head]
        inc al
        mov [target_head], al

        cmp al, [FloppyInfoStruct.head_max]
        jg load_boot2_1_floppy.over_head
        jmp load_boot2_1_floppy.valid_sector

    .over_head:
        mov [target_head], 0

        mov al, [target_cylinder]
        inc al
        mov [target_cylinder], al

    .valid_sector:

    xor ax, ax
    mov ds, ax
    mov ch, [target_cylinder] ; cylinder
    mov cl, [target_sector] ; sector
    and cl, 0x3F
    mov al, [target_cylinder]
    shr al, 2
    and al, 0xC0
    or cl, al
    mov dh, [target_head] ; head
    mov dl, [drive_boot]
    mov dword eax, [floppy_target_address]
    shr dword eax, 4
    mov es, ax
    xor bx, bx
    mov ah, 0x2
    mov al, 1 ; sectors to read
    int 0x13
    jnc load_boot2_1_floppy.floppy_success

    dec si
    jnz load_boot2_1_floppy.retry
    jmp fail_floppy

    .floppy_success:
        mov al, [target_sector]
        add al, 1
        mov [target_sector], al

        mov al, [floppy_read_loop]
        dec al
        mov [floppy_read_loop], al

        mov dword eax, [floppy_target_address]
        add eax, 512
        mov dword [floppy_target_address], eax

        jmp load_boot2_1_floppy.loop
    .loop_success:
        mov al, '1'
        mov ah, 0x0E
        int 0x10

        ret

fail_floppy:
    mov al, '0'
    mov ah, 0x0E
    int 0x10
    mov al, 'F'
    mov ah, 0x0E
    int 0x10

    ; get the last status
    mov ah, 0x01
    mov dl, [drive_boot]
    int 0x13
    mov [floppy_return_code], ah

    xor eax, eax
    mov al, [floppy_return_code]
    xor ebx, ebx
    xor ecx, ecx
    xor edx, edx
    mov bl, [target_cylinder]
    mov cl, [FloppyInfoStruct.head_max]
    mov dl, [target_sector]

    jmp halt

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
        jmp halt

    .upper_memory_done:  
        mov eax, [e820_cur_offset]
        mov [0xEFE8], eax

        ret

read_error:
    mov al, 'F'
    mov ah, 0x0E
    int 0x10
    mov al, 'H'
    mov ah, 0x0E
    int 0x10
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


    ;jmp halt

    ret

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
    mov ax, 0xE001
    mov dx, [VbeFlags]
    mov cx, [filter_run_count]
    jmp halt

vbe_errorm:
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




memory_type: resb 1
upper_memory: resq 1
set_mode: resb 2
drive_boot: resb 1

floppy_read_loop: resb 1

target_cylinder: resb 2
target_head: resb 1
target_sector: resb 1

floppy_target_address: resb 4

floppy_return_code: resb 1

filter_run_count: resb 2

vbe_signature: db 'VBE2'
vbe_version: db 0x0200

VbeFlags: resb 1

cur_row: resb 2
cur_col: resb 2

lba_chs_arg: resb 2

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

times 4090-($-$$) db 0
; struct should be at physical 0x1FFA with a size of 6 bytes
FloppyInfoStruct:
    .drive_count: resb 1
    .cylinder_max: resb 2
    .head_max: resb 1
    .sector_max: resb 1

    .head_count: resb 1