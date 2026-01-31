[org 0x7C00]
[BITS 16]

mov [drive_boot], dl

mov al, '1'
mov ah, 0x0E
int 0x10

align 16
dap:
    db 0x10
    db 0x00
    dw 8
    dw 0x1000
    dw 0x0000
    dq 1

start:
    cli ; Disable Interrupts
    jmp segment_registers

segment_registers:
    ; Clear Segment Registers
    xor ax, ax
    mov ss, ax
    mov sp, 0x7000 ; set stack pointer
    mov ds, ax
    mov es, ax
    sti ; re-enable interrupts

    mov ah, 0x1
    mov cx, 0x2607
    int 0x10 ; hide cursor

    mov ah, 0x00
    mov dl, [drive_boot]
    int 0x13 ; reset drive

    call get_floppy_info

    ; find out if the BIOS is treating the (presumably) USB as HDD or FDD
    mov dl, [drive_boot]
    cmp dl, 0x80
    jl read_boot1_floppy
    cmp dl, 0xFF
    jl read_boot1_hdd
    jmp halt

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

read_boot1_hdd:
    mov al, 'H'
    mov ah, 0x0E
    int 0x10

    mov si, 3
    .retry_hdd:

    mov ah, 0x00
    mov dl, [drive_boot]
    int 0x13 ; reset drive

    ; load stage 2
    mov ah, 0x42
    mov dl, [drive_boot]
    mov si, dap
    int 0x13

    jnc read_boot1_hdd.hdd_success

    dec si
    jnz read_boot1_hdd.retry_hdd
    jmp fail_hdd

    .hdd_success:
        mov al, '1'
        mov ah, 0x0E
        int 0x10

        mov dl, [drive_boot]
        jmp 0x0000:0x1000

read_boot1_floppy:
    mov al, 'F'
    mov ah, 0x0E
    int 0x10

    ; starting LBA = 600
    mov ax, 1
    call calculate_LBA_to_CHS

    mov dword [floppy_target_address], 0x1000

    mov [floppy_read_loop], 8 ; read 8 sectors

    .loop:
    mov al, [floppy_read_loop]
    cmp al, 0
    je read_boot1_floppy.loop_success

    mov si, 3 ; for each sector read, we can retry 3 times
    .retry:

    mov al, [target_sector]
    cmp al, [FloppyInfoStruct.sector_max]
    jg read_boot1_floppy.over_sector
    jmp read_boot1_floppy.valid_sector

    .over_sector:
        ; correct the sector count
        mov al, [target_sector]
        sub al, [FloppyInfoStruct.sector_max]
        mov [target_sector], al

        mov al, [target_head]
        inc al
        mov [target_head], al

        cmp al, [FloppyInfoStruct.head_max]
        jg read_boot1_floppy.over_head
        jmp read_boot1_floppy.valid_sector

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
    jnc read_boot1_floppy.floppy_success

    dec si
    jnz read_boot1_floppy.retry
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

        jmp read_boot1_floppy.loop
    .loop_success:
        mov al, '1'
        mov ah, 0x0E
        int 0x10

        jmp 0x0000:0x1000

fail_floppy:
    mov al, '0'
    mov ah, 0x0E
    int 0x10
    jmp halt

fail_hdd:
    mov al, '0'
    mov ah, 0x0E
    int 0x10
    jmp halt

halt:
    hlt
    jmp halt


; AX should be the number to print
print_int:
    mov cx, 0
    mov bx, 10

    .divide_loop:
        xor dx, dx
        div bx
        push dx ; the remainder
        inc cx
        cmp ax, 0
        jne .divide_loop

    .print_loop:
        pop ax
        add al, '0'
        mov ah, 0x0E
        int 0x10
        dec cx
        cmp cx, 0
        jne .print_loop

    ret

drive_boot: resb 1
target_cylinder: resb 1
target_head: resb 1
target_sector: resb 1

floppy_target_address: resb 4
floppy_read_loop: resb 1

FloppyInfoStruct:
    .drive_count: resb 1
    .cylinder_max: resb 2
    .head_max: resb 1
    .sector_max: resb 1

    .head_count: resb 1

times 446-($-$$) db 0

; This table should ignore CHS, so it's set to invalid values
Paritition_Table:
    .mbr_partition:
        db 0x80              ; bootable
        db 0xFE              ; start head
        db 0xFF              ; start sector + high cyl bits
        db 0xFF              ; start cylinder
        db 0x83              ; partition type
        db 0xFE              ; end head
        db 0xFF              ; end sector + high cyl bits
        db 0xFF              ; end cylinder
        dd 0x00000000        ; starting LBA (after MBR)
        dd 0x00000800        ; sectors (2048 sectors = 1MB)
    .user_partition:
        db 0x00              ; not bootable
        db 0xFE
        db 0xFF
        db 0xFF
        db 0x83              ; custom partition type
        db 0xFE
        db 0xFF
        db 0xFF
        dd 0x00001000        ; start at sector 4096
        dd 0x00100000        ; size: pick something reasonable (example: 1M sectors)
    .partition_3:
        dq 0x00
        dq 0x00
    .partition_4:
        dq 0x00
        dq 0x00

times 510-($-$$) db 0
dw 0xAA55