; ============================================================
; crash.qa —— 故意写 NULL 触发 page fault，演示任务隔离
; 文件 : userland/apps/crash/crash.asm
; ============================================================
BITS 32
org 0

%include "userland/libyua/yua.inc"

_start:
    push ebx
    push esi
    push edi
    push ebp

    mov ebx, [esp + 20]        ; rt
    mov ebp, [esp + 24]        ; base

    lea ecx, [ebp + msg1 - _start]
    push ecx
    call dword [ebx + RT_PRINT]
    add esp, 4

    ; 写 NULL —— 触发 #PF
    mov eax, 0
    mov dword [eax], 0xDEADBEEF

    ; 永不返回

    pop ebp
    pop edi
    pop esi
    pop ebx
    ret

msg1: db "crash.qa: about to write to NULL...", 10, 0
