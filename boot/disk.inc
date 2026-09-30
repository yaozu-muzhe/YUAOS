; ============================================================
; YUAOS - 磁盘读取公共例程（实模式）
; 文件 : boot/disk.inc
; 用法 : 在 BITS 16 的源文件中 %include "disk.inc"
; ============================================================

; ------------------------------------------------------------
; read_sectors
; 输入: AX = 起始 LBA, CX = 扇区数, DL = 驱动器号
;       ES:BX = 目标缓冲区
; 输出: CF = 0 成功 / CF = 1 失败
; 说明: 逐扇区读取，每扇区失败重试 5 次，失败后复位磁盘；
;       自动处理 ES 段跨越 64KB 边界；内部保存并恢复 ES。
; ------------------------------------------------------------
read_sectors:
    pusha
    push es

    mov [rs_lba], ax
    mov [rs_cnt], cx
    mov [rs_dst], bx
    mov [rs_seg], es
    mov [rs_drv], dl

.rs_loop:
    mov byte [rs_try], 5
.rs_attempt:
    mov ax, [rs_lba]
    call lba_to_chs

    mov ax, [rs_seg]
    mov es, ax
    mov bx, [rs_dst]
    mov dl, [rs_drv]
    mov ah, 0x02
    mov al, 1
    int 0x13
    jnc .rs_ok

    ; 复位磁盘控制器后重试
    pusha
    xor ah, ah
    mov dl, [rs_drv]
    int 0x13
    popa

    dec byte [rs_try]
    jnz .rs_attempt
    jmp .rs_fail

.rs_ok:
    inc word [rs_lba]
    add word [rs_dst], 512
    jnc .rs_nowrap
    mov ax, [rs_seg]
    add ax, 0x1000
    mov [rs_seg], ax
.rs_nowrap:
    dec word [rs_cnt]
    jnz .rs_loop

    pop es
    popa
    clc
    ret

.rs_fail:
    pop es
    popa
    stc
    ret

; ------------------------------------------------------------
; lba_to_chs
; 输入: AX = LBA
; 输出: CH = 柱面, CL = 扇区(1-based), DH = 磁头
; 破坏: AX, BX, DX
; 说明: 软盘几何 1.44MB = 18 扇区/道, 2 磁头, 80 柱面
; ------------------------------------------------------------
lba_to_chs:
    xor dx, dx
    mov bx, 36
    div bx                  ; AX = 柱面, DX = LBA % 36
    mov ch, al

    mov ax, dx
    xor dx, dx
    mov bx, 18
    div bx                  ; AX = 磁头, DX = LBA % 18
    mov dh, al
    mov cl, dl
    inc cl
    ret

; ------------------------------------------------------------
; 内部变量
; ------------------------------------------------------------
rs_lba  dw 0
rs_cnt  dw 0
rs_dst  dw 0
rs_seg  dw 0
rs_drv  db 0
rs_try  db 0
