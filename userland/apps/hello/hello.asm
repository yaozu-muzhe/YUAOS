; ============================================================
; hello.qa —— YUAOS userland demo
; 文件 : userland/apps/hello/hello.asm
; 汇编 : nasm -f bin
;
; 重要：本程序作为 C 函数被 qa_load_and_run 调用，
;       必须遵守 cdecl 约定：保护 ebx/esi/edi/ebp。
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

    ; 现在的栈布局（S = 进入时的 esp）：
    ;   [esp+ 0] = 旧 ebp
    ;   [esp+ 4] = 旧 edi
    ;   [esp+ 8] = 旧 esi
    ;   [esp+12] = 旧 ebx
    ;   [esp+16] = 返回地址
    ;   [esp+20] = rt  (qa_runtime_t*)
    ;   [esp+24] = base (u8*)

    mov ebx, [esp + 20]        ; ebx = rt
    mov ebp, [esp + 24]        ; ebp = base

    ; ---- rt->print("Hello from source!\n") ----
    lea ecx, [ebp + msg1 - _start]
    push ecx
    call dword [ebx + RT_PRINT]
    add esp, 4

    ; ---- rt->putc('Y') ----
    push 'Y'
    call dword [ebx + RT_PUTC]
    add esp, 4

    ; ---- rt->putc('\n') ----
    push 10
    call dword [ebx + RT_PUTC]
    add esp, 4

    ; ---- rt->print_hex(0xCAFEBABE) ----
    push 0xCAFEBABE
    call dword [ebx + RT_PRINT_HEX]
    add esp, 4

    ; ---- rt->print("\n") ----
    lea ecx, [ebp + nl - _start]
    push ecx
    call dword [ebx + RT_PRINT]
    add esp, 4

    ; ---- rt->sleep(500) ----
    push 500
    call dword [ebx + RT_SLEEP]
    add esp, 4

    ; ---- rt->print("slept 500 ms, exiting.\n") ----
    lea ecx, [ebp + msg2 - _start]
    push ecx
    call dword [ebx + RT_PRINT]
    add esp, 4

    ; ---- 恢复 callee-saved 寄存器（FILO）----
    pop ebp
    pop edi
    pop esi
    pop ebx

    ; ---- 返回加载器 ----
    xor eax, eax
    ret

msg1: db "Hello from source!", 10, 0
nl:   db 10, 0
msg2: db "slept 500 ms, exiting.", 10, 0
