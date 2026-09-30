; ============================================================
; YUAOS - 第二阶段引导（16 号：VBE + 字库）
; 文件 : boot/stage2.asm
; ============================================================

[BITS 16]
[ORG 0x8000]

KERNEL_LBA   equ 9
KERNEL_CNT   equ 512
KERNEL_SEG   equ 0x1000
KERNEL_OFF   equ 0x0000
KERNEL_ADDR  equ 0x10000

E820_COUNT_ADDR   equ 0x9000
E820_ENTRIES_ADDR equ 0x9004
E820_END_ADDR     equ 0xA000

VBE_INFO_ADDR   equ 0x0700
VBE_MODE_ADDR   equ 0x0900
VBE_RESULT_ADDR equ 0x0A00
FONT_ROM_ADDR   equ 0x0B00

stage2_entry:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti

    mov [boot_drive], dl

    mov si, msg_stage2
    call print_string



    ; ---- 2. E820 ----
    call detect_e820

    ; ---- 3. VBE ----
    mov si, msg_vbe
    call print_string
    call detect_vbe

    mov si, msg_vbe_ok
    call print_string

    ; ---- 4. 读内核 ----
    mov ax, KERNEL_SEG
    mov es, ax
    mov bx, KERNEL_OFF
    mov ax, KERNEL_LBA
    mov cx, KERNEL_CNT
    mov dl, [boot_drive]
    call read_sectors
    jc .disk_err

    mov si, msg_kernel_ok
    call print_string

    ; ---- 5. A20 ----
    call enable_a20
    jc .a20_err
    mov si, msg_a20_ok
    call print_string

    ; ---- 6. 进保护模式 ----
    cli
    lgdt [gdt_descriptor]
    mov eax, cr0
    or  eax, 0x00000001
    mov cr0, eax
    jmp 0x08:pm_entry

.disk_err:
    mov si, msg_disk_err
    call print_string
    jmp halt16

.a20_err:
    mov si, msg_a20_err
    call print_string
    jmp halt16

; ============================================================
; 保存 8x16 字库
; 源 : F000:FA6E  （IBM PC 标准 8x16 字库位置）
; 目标 : 0x0000:0B00，共 4KB（256 字符 × 16 字节）
; ============================================================
save_font_rom:
    pusha
    push ds
    push es

    mov ax, 0xF000
    mov ds, ax
    mov si, 0xFA6E

    xor ax, ax
    mov es, ax
    mov di, FONT_ROM_ADDR

    mov cx, 2048                ; 4KB / 2 字节
    cld
    rep movsw

    pop es
    pop ds
    popa
    ret

; ============================================================
; VBE 探测 + 设置 800x600x32
; ============================================================
detect_vbe:
    pusha
    push es
    push fs
    push ds

    xor ax, ax
    mov es, ax
    mov ds, ax

    mov dword [VBE_INFO_ADDR], 'VESA'
    mov dword [VBE_INFO_ADDR+4], 0
    mov di, VBE_INFO_ADDR
    mov ax, 0x4F00
    int 0x10
    cmp ax, 0x004F
    jne .fail

    mov eax, [VBE_INFO_ADDR]
    cmp eax, 'VESA'
    jne .fail

    mov si, [VBE_INFO_ADDR + 0x0E]
    mov ax, [VBE_INFO_ADDR + 0x10]
    mov fs, ax

.next_mode:
    mov cx, [fs:si]
    cmp cx, 0xFFFF
    je .fail
    add si, 2

    push si
    push fs
    xor ax, ax
    mov es, ax
    mov di, VBE_MODE_ADDR
    mov ax, 0x4F01
    int 0x10
    pop fs
    pop si
    cmp ax, 0x004F
    jne .next_mode

    mov ax, [VBE_MODE_ADDR + 0x12]
    cmp ax, 800
    jne .next_mode
    mov ax, [VBE_MODE_ADDR + 0x14]
    cmp ax, 600
    jne .next_mode
    mov al, [VBE_MODE_ADDR + 0x19]
    cmp al, 32
    jne .next_mode

    mov bx, cx
    or bx, 0x4000
    mov ax, 0x4F02
    int 0x10
    cmp ax, 0x004F
    jne .fail

    xor ax, ax
    mov es, ax

    mov eax, [VBE_MODE_ADDR + 0x28]
    mov [VBE_RESULT_ADDR + 0], eax

    mov ax, [VBE_MODE_ADDR + 0x12]
    mov [VBE_RESULT_ADDR + 4], ax

    mov ax, [VBE_MODE_ADDR + 0x14]
    mov [VBE_RESULT_ADDR + 6], ax

    mov ax, [VBE_MODE_ADDR + 0x10]
    mov [VBE_RESULT_ADDR + 8], ax

    mov al, [VBE_MODE_ADDR + 0x19]
    mov [VBE_RESULT_ADDR + 10], al

    mov byte [VBE_RESULT_ADDR + 11], 1

    pop ds
    pop fs
    pop es
    popa
    ret

.fail:
    xor ax, ax
    mov es, ax
    mov byte [VBE_RESULT_ADDR + 11], 0
    pop ds
    pop fs
    pop es
    popa
    ret

; ============================================================
; 32 位保护模式入口
; ============================================================
[BITS 32]
pm_entry:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x90000

    jmp 0x08:KERNEL_ADDR

.halt32:
    cli
    hlt
    jmp .halt32

; ============================================================
[BITS 16]

halt16:
    cli
    hlt
    jmp halt16

detect_e820:
    push es
    push ds
    xor ax, ax
    mov es, ax
    mov ds, ax
    mov dword [E820_COUNT_ADDR], 0
    mov di, E820_ENTRIES_ADDR
    xor ebx, ebx
    xor bp, bp
.loop:
    mov eax, 0xE820
    mov edx, 0x534D4150
    mov ecx, 24
    int 0x15
    jc .done
    cmp eax, 0x534D4150
    jne .done
    inc dword [E820_COUNT_ADDR]
    add di, 24
    inc bp
    cmp bp, 128
    jae .done
    cmp di, E820_END_ADDR
    jae .done
    test ebx, ebx
    jnz .loop
.done:
    pop ds
    pop es
    ret

enable_a20:
    in  al, 0x92
    test al, 0x02
    jnz .done
    or  al, 0x02
    and al, 0xFE
    out 0x92, al
.done:
    clc
    ret

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

%include "disk.inc"

gdt_start:
    dq 0
gdt_code:
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 10011010b
    db 11001111b
    db 0x00
gdt_data:
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 10010010b
    db 11001111b
    db 0x00
gdt_end:
gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

boot_drive  db 0

msg_stage2      db "Stage2: saving font ROM...", 13, 10, 0
msg_vbe         db "Stage2: probing VBE...", 13, 10, 0
msg_vbe_ok      db "Stage2: VBE mode 800x600x32 set.", 13, 10, 0
msg_kernel_ok   db "Stage2: kernel loaded.", 13, 10, 0
msg_a20_ok      db "Stage2: A20 enabled.", 13, 10, 0
msg_disk_err    db "Disk Error >.<", 13, 10, 0
msg_a20_err     db "A20 Error >.<", 13, 10, 0
