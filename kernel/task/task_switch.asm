; ============================================================
; YUAOS - 任务上下文切换
; 文件 : kernel/task/task_switch.asm
; ============================================================

[BITS 32]

global setjmp_
global longjmp_
global task_call

section .text

; ------------------------------------------------------------
; int setjmp_(jmp_buf_t* env);
;   jmp_buf 布局：
;     [0]  ebx
;     [4]  esi
;     [8]  edi
;     [12] ebp
;     [16] esp   (call 之后、pop 参数之前)
;     [20] eip   (返回地址)
;     [24] eflags
; ------------------------------------------------------------
setjmp_:
    mov eax, [esp+4]            ; env
    mov [eax+0],  ebx
    mov [eax+4],  esi
    mov [eax+8],  edi
    mov [eax+12], ebp
    lea ecx, [esp+4]            ; esp 在返回地址弹出之后
    mov [eax+16], ecx
    mov ecx, [esp]              ; 返回地址
    mov [eax+20], ecx
    pushfd
    pop ecx
    mov [eax+24], ecx
    xor eax, eax                ; 首次返回 0
    ret

; ------------------------------------------------------------
; void longjmp_(jmp_buf_t* env, int val);
; ------------------------------------------------------------
longjmp_:
    mov eax, [esp+4]
    mov edx, [esp+8]
    mov ebx, [eax+0]
    mov esi, [eax+4]
    mov edi, [eax+8]
    mov ebp, [eax+12]
    mov ecx, [eax+20]           ; eip
    mov esp, [eax+16]           ; esp（新栈）
    push dword [eax+24]         ; 恢复 eflags
    popfd
    mov eax, edx
    test eax, eax
    jnz .ok
    mov eax, 1                  ; val == 0 时返回 1
.ok:
    jmp ecx

; ------------------------------------------------------------
; u32 task_call(u32 new_esp,
;               void (*fn)(void*, void*),
;               void* a, void* b);
;   切换到 new_esp，调用 fn(a, b)；fn 返回后切回原栈。
; ------------------------------------------------------------
task_call:
    push ebp
    push ebx
    push esi
    push edi

    mov eax, [esp+20]           ; new_esp
    mov ecx, [esp+24]           ; fn
    mov edx, [esp+28]           ; a
    mov esi, [esp+32]           ; b

    mov ebx, esp                ; 保存当前（调度器）esp

    mov esp, eax                ; 切到任务栈
    push esi                    ; b
    push edx                    ; a
    call ecx                    ; fn(a, b)
    add esp, 8

    mov esp, ebx                ; 切回调度器栈

    pop edi
    pop esi
    pop ebx
    pop ebp
    xor eax, eax
    ret
