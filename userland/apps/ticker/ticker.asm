; ============================================================
; ticker.qa —— 每 200 ms 打一个点，共 5 个
; 文件 : userland/apps/ticker/ticker.asm
;
; 同样必须保护 callee-saved 寄存器（ebx/esi/edi/ebp）。
; ============================================================
BITS 32
org 0

%include "userland/libyua/yua.inc"

_start:
    ; ---- 保护 callee-saved 寄存器 ----
    push ebx
    push esi
    push edi
    push ebp

    ; 栈布局同 hello.asm：
    ;   [esp+20] = rt
    ;   [esp+24] = base

    mov ebx, [esp + 20]        ; ebx = rt
    mov ebp, [esp + 24]        ; ebp = base
    mov esi, 5                 ; 循环 5 次（esi 也是 callee-saved，已保护）

.loop:
    lea ecx, [ebp + dot - _start]
    push ecx
    call dword [ebx + RT_PRINT]
    add esp, 4

    push 200
    call dword [ebx + RT_SLEEP]
    add esp, 4

    dec esi
    jnz .loop

    lea ecx, [ebp + done - _start]
    push ecx
    call dword [ebx + RT_PRINT]
    add esp, 4

    ; ---- 恢复 callee-saved 寄存器 ----
    pop ebp
    pop edi
    pop esi
    pop ebx

    ; ---- 返回加载器 ----
    xor eax, eax
    ret

dot:  db ".", 0
done: db 10, "ticker done.", 10, 0
