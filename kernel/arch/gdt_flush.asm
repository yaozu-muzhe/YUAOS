; ============================================================
; YUAOS - GDT 加载
; 文件 : kernel/arch/gdt_flush.asm
; 作用 : lgdt 之后用远跳转刷新 CS
; ============================================================

[BITS 32]

global gdt_flush

section .text

gdt_flush:
    mov eax, [esp + 4]
    lgdt [eax]

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    jmp 0x08:gdt_flush_ret

gdt_flush_ret:
    ret
