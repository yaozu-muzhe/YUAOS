; ============================================================
; YUAOS - 内核入口
; 文件 : kernel/entry.asm
; 格式 : ELF32 目标文件
; ============================================================

[BITS 32]

global _start
extern kernel_main
extern __bss_start
extern __bss_end

section .text.entry progbits alloc exec nowrite

_start:
    cli

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; ---- 内核栈：0x9FC00 向下生长（BIOS EBDA 之前）----
    mov ss, ax
    mov esp, 0x9FC00
    mov ebp, esp

    ; ---- 清零 BSS ----
    mov edi, __bss_start
    mov ecx, __bss_end
    sub ecx, edi
    xor eax, eax
    cld
    rep stosb

    call kernel_main

.halt:
    cli
    hlt
    jmp .halt
