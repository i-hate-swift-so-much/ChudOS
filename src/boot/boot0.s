[org 0x7C00]
[BITS 16]

mov [drive_boot], dl

mov al, '1'
mov ah, 0x0E
int 0x10
mov al, ' '
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

    mov ax, [drive_boot]
    call print_int

    mov al, ' '
    mov ah, 0x0E
    int 0x10

    ; find out if the BIOS is treating the (presumably) USB as HDD or FDD
    mov dl, [drive_boot]
    cmp dl, 0
    je read_boot1_floppy

    mov dl, [drive_boot]
    cmp dl, 0x80
    je read_boot1_hdd

    mov dl, [drive_boot]
    cmp dl, 0x7F
    jg read_boot1_hdd

    mov dl, [drive_boot]
    cmp dl, 0
    jg read_boot1_floppy

    jmp halt

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
    jmp drive_fail

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
    mov [target_head], 0
    mov [target_sector], 2

    mov dword [floppy_target_address], 0x1000

    mov si, 3 ; for each sector read, we can retry 3 times
    .retry:

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
    mov al, 8 ; sectors to read
    int 0x13
    jnc read_boot1_floppy.success

    mov [last_error_code], ah

    dec si
    jnz read_boot1_floppy.retry
    jmp drive_fail

    .success:
        mov al, '1'
        mov ah, 0x0E
        int 0x10

        jmp 0x0000:0x1000

drive_fail:
    mov al, '0'
    mov ah, 0x0E
    int 0x10

    mov al, ' '
    mov ah, 0x0E
    int 0x10

    xor ax, ax
    mov al, [last_error_code]
    call print_int

    mov al, ' '
    mov ah, 0x0E
    int 0x10

    xor ax, ax
    mov al, [target_cylinder]
    call print_int

    mov al, ':'
    mov ah, 0x0E
    int 0x10

    xor ax, ax
    mov al, [target_head]
    call print_int

    mov al, ':'
    mov ah, 0x0E
    int 0x10

    xor ax, ax
    mov al, [target_sector]
    call print_int

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
last_error_code: resb 1

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
        dd 0x00000800        ; starting LBA
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
        dd 0x00001000        ; start at sector 2048 (some bioses want that)
        dd 0x00100000        ; size: pick something reasonable (example: 1M sectors)
    .partition_3:
        dq 0x00
        dq 0x00
    .partition_4:
        dq 0x00
        dq 0x00

times 510-($-$$) db 0
dw 0xAA55