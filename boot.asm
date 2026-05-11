; jatiN Corporation Bootloader
; Simple bootloader that displays "jatiN corporation" text
; Created by Jatin - Age 10

[BITS 16]
[ORG 0x7C00]

; Main boot code
start:
    ; Set up segments
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    
    ; Clear screen
    mov ax, 0x0003
    int 0x10
    
    ; Set video mode to 80x25 text mode
    mov ax, 0x0003
    int 0x10
    
    ; Set background color (blue)
    mov ax, 0x0600
    mov bh, 0x1A    ; Blue background
    mov cx, 0x0000  ; Upper left
    mov dx, 0x184F  ; Lower right (80x25)
    int 0x10
    
    ; Print welcome message
    mov si, welcome_msg
    call print_string
    
    ; Print jatiN corporation text (main display)
    mov si, jatin_main
    call print_string_centered
    
    ; Print tagline
    mov si, tagline
    call print_string
    
    ; Print version info
    mov si, version_info
    call print_string
    
    ; Print boot instructions
    mov si, boot_instructions
    call print_string
    
    ; Blink cursor
    mov ah, 0x02
    mov bh, 0x00
    mov dx, 0x1500  ; Row 21, Col 0
    int 0x10
    
    mov ah, 0x01
    mov ch, 0x06    ; Blinking block cursor
    mov cl, 0x07
    int 0x10
    
    ; Halt
    cli
    hlt

; Print string function
print_string:
    pusha
.print_loop:
    lodsb
    cmp al, 0
    je .done
    mov ah, 0x0E
    mov bh, 0x00
    mov bl, 0x0F    ; White text
    int 0x10
    jmp .print_loop
.done:
    popa
    ret

; Print string centered
print_string_centered:
    pusha
    mov ah, 0x03    ; Get cursor position
    mov bh, 0x00
    int 0x10
    
    ; Calculate center position
    mov ah, 0x0F    ; Get current video mode
    int 0x10
    mov ah, 0x03
    mov bh, 0x00
    int 0x10        ; DH = row, DL = col
    
    ; Center horizontally (80 cols - string length) / 2
    mov cx, 40      ; Approximate center
    
.center_loop:
    lodsb
    cmp al, 0
    je .done_center
    mov ah, 0x0E
    mov bh, 0x00
    mov bl, 0x0E    ; Yellow text for main title
    int 0x10
    jmp .center_loop
.done_center:
    popa
    ret

; Print new line
print_newline:
    pusha
    mov ah, 0x0E
    mov al, 0x0D    ; Carriage return
    mov bh, 0x00
    mov bl, 0x0F
    int 0x10
    mov al, 0x0A    ; Line feed
    int 0x10
    popa
    ret

; Data section
welcome_msg db '========================================', 0x0D, 0x0A, 0

jatin_main db '    jatiN corporation', 0x0D, 0x0A, 0

tagline db 0x0D, 0x0A
          db '    Innovation Beyond Limits', 0x0D, 0x0A
          db '    Founded by Jatin (Age 10)', 0x0D, 0x0A
          db 0

version_info db 0x0D, 0x0A
             db '    Bootloader Version: 1.0.0', 0x0D, 0x0A
             db '    (C) 2026 jatiN Corporation', 0x0D, 0x0A
             db 0

boot_instructions db 0x0D, 0x0A
                  db '    Press F11 for BIOS Setup', 0x0D, 0x0A
                  db '    Press any key to continue...', 0x0D, 0x0A
                  db 0

; Boot sector padding and signature
times 510 - ($ - $$) db 0
dw 0xAA55
