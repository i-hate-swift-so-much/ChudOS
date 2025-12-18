[org 0x7C00]
[BITS 16]

mov ah, 0x0
mov al, 0x3
int 0x10                ; text mode

mov ah, 0x1
mov cx, 0x2607
int 0x10 ; hide cursor

dap:
    db 0x10
    db 0x00
    dw 2
    dw 0x1000
    dw 0x0000
    dq 1

start:
    cli ; Disable Interrupts
    jmp segment_registers

segment_registers:
    ; Clear Segment Registers
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00 ; Set stack pointer
    sti ; Enable Interrupts

    mov [drive_boot], dl
    mov ah, 0x00
    mov dl, [drive_boot]
    int 0x13

read_boot1:
    call clear_screen
    mov si, boot_read_indicator
    call print
    mov al, dl
    call print_hex_dx

    ; load stage 2
    mov ah, 0x42
    mov dl, [drive_boot]
    mov si, dap
    int 0x13

    jc read_error
    mov si, boot_msg_jmp
    call print

    jmp 0x1000

read_error:
    cmp dl, 0x00
    je fail_floppy
    cmp dl, 0x80
    je fail_hdd
    jmp halt

fail_floppy:
    mov si, boot_err_read_floppy
    call print
    mov al, ah
    call print_hex_dx
    jmp halt

fail_hdd:
    mov si, boot_err_read_hdd
    call print
    mov al, ah
    call print_hex_dx
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

print_hex_dx:
    pusha           ; Save registers
    mov ax, dx      ; Copy DX to AX for manipulation

    ; Print MSB nibble
    shr ax, 12      ; Get the most significant nibble
    call convert_and_print_nibble

    ; Print next nibble
    mov ax, dx      ; Restore original value
    shr ax, 8       ; Get the next nibble
    and ax, 0x000F  ; Mask to isolate current nibble
    call convert_and_print_nibble

    ; Print next nibble
    mov ax, dx      ; Restore original value
    shr ax, 4       ; Get the next nibble
    and ax, 0x000F  ; Mask to isolate current nibble
    call convert_and_print_nibble

    ; Print LSB nibble
    mov ax, dx      ; Restore original value
    and ax, 0x000F  ; Get the least significant nibble
    call convert_and_print_nibble

    popa            ; Restore registers
    mov si, newline_terminator
    call print
    ret

convert_and_print_nibble:
    cmp al, 0x0A    ; Check if it's a number or letter
    jl is_num
    add al, 0x07    ; Adjust for 'A'-'F' range
is_num:
    add al, 0x30    ; Convert to ASCII character ('0'-'9')
    mov ah, 0x0E    ; BIOS Teletype output function
    int 0x10        ; Call BIOS interrupt
    ret

newline_terminator: db 0x0a,0x0D
boot_msg_jmp: db "Succesfully read Boot1 from HDD, jumping to Boot1",0x0A,0x0D,0
boot_err_read_hdd: db "Failed to Read Boot1 from HDD. BIOS said:",0x0A,0x0D,0
boot_read_indicator: db "Drive: ",0x0A,0x0D,0
boot_err_read_floppy: db "Failed to Read Boot1 from Floppy. BIOS said:",0x0A,0x0D,0
drive_err7_msg: db "Drive error 7",0x0A,0x0D,0
drive_boot: resb 1

times 446-($-$$) db 0

; This table should ignore CHS, so it's set to invalid values
Paritition_Table:
    .kernel_partition:
        db 0x80              ; bootable
        db 0xFE              ; start head
        db 0xFF              ; start sector + high cyl bits
        db 0xFF              ; start cylinder
        db 0xA0              ; custom OS partition type
        db 0xFE              ; end head
        db 0xFF              ; end sector + high cyl bits
        db 0xFF              ; end cylinder
        dd 0x00000001        ; starting LBA (after MBR)
        dd 0x00000800        ; sectors (2048 sectors = 1MB)
    .user_partition:
        db 0x00              ; not bootable
        db 0xFE
        db 0xFF
        db 0xFF
        db 0xA0              ; custom partition type
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