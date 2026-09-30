; ============================================================
; YUAOS - 引导扇区
; 文件 : boot/boot.asm
; 架构 : i686 / 16 位实模式
; 作用 : BIOS 载入 0x7C00 后接管；
;        把 stage2 从磁盘读入 0x0000:0x8000 并跳转。
; ============================================================

[BITS 16]
[ORG 0x7C00]

; ---- 磁盘布局（LBA 单位，1 扇区 = 512B）----
STAGE2_LBA  equ 1
STAGE2_CNT  equ 8           ; 4 KB
STAGE2_SEG  equ 0x0000
STAGE2_OFF  equ 0x8000

_start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti

    mov [boot_drive], dl    ; BIOS 传入的启动驱动器号

    mov si, msg_boot
    call print_string

    ; ---- 读取 stage2 到 0x0000:0x8000 ----
    mov ax, STAGE2_SEG
    mov es, ax
    mov bx, STAGE2_OFF
    mov ax, STAGE2_LBA
    mov cx, STAGE2_CNT
    mov dl, [boot_drive]
    call read_sectors
    jc .disk_err

    mov si, msg_ok
    call print_string

    ; ---- 跳入 stage2 ----
    mov dl, [boot_drive]    ; 传递驱动器号给 stage2
    jmp STAGE2_SEG:STAGE2_OFF

.disk_err:
    mov si, msg_err
    call print_string
.halt:
    cli
    hlt
    jmp .halt

; ---- 打印以 0 结尾的字符串 ----
print_string:
    pusha
    mov ah, 0x0E
.next:
    lodsb
    test al, al
    jz .done
    int 0x10
    jmp .next
.done:
    popa
    ret

; ---- 磁盘读取公共例程 ----
%include "disk.inc"

; ---- 数据 ----
boot_drive db 0

msg_boot db "YUAOS Bootloader v1.0", 13, 10
         db "Welcome YUAOS     >.<", 13, 10, 0
msg_ok   db "Stage2 loaded, jumping...", 13, 10, 0
msg_err  db "Disk Error >.<", 13, 10, 0

times 510 - ($ - $$) db 0
dw 0xAA55
