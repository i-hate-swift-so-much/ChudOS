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

    ; find out if the BIOS is treating the (presumably) USB as HDD or FDD
    mov dl, [drive_boot]
    cmp dl, 0x80
    je read_boot1_hdd
    cmp dl, 0x00
    je read_boot1_floppy


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

    mov [target_cylinder], 0
    mov [target_sector], 2
    mov [target_head], 0

    mov si, 3
    .retry_floppy:

    mov ah, 0x00
    mov dl, [drive_boot]
    int 0x13 ; reset drive
    clc

    mov ch, [target_cylinder] ; cylinder
    mov cl, [target_sector] ; sector
    and cl, 0x3F
    mov al, [target_cylinder]
    shr al, 2
    and al, 0xC0
    or cl, al
    mov dh, [target_head] ; head
    mov dl, [drive_boot]
    xor ax, ax
    mov es, ax
    mov bx, 0x1000
    mov ah, 0x2
    mov al, 8
    int 0x13
    jnc read_boot1_floppy.floppy_success

    dec si
    jnz read_boot1_floppy.retry_floppy
    jmp fail_floppy
    
    .floppy_success:
        mov al, '1'
        mov ah, 0x0E
        int 0x10

        mov dl, [drive_boot]
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

halt:
    hlt
    jmp halt

drive_boot: resb 1
target_cylinder: resb 1
target_head: resb 1
target_sector: resb 1

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