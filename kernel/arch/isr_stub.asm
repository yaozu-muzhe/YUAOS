; ============================================================
; YUAOS - 异常 + IRQ 汇编桩
; 文件 : kernel/arch/isr_stub.asm
;
; 栈帧布局（从低地址到高地址）：
;   pusha: edi esi ebp esp ebx edx ecx eax
;   桩   : int_no err_code
;   CPU  : eip cs eflags [useresp ss]
; ============================================================

[BITS 32]

section .text

extern isr_handler
extern irq_handler

; ---- 宏 ----
%macro ISR_NOERR 1
global isr%1
isr%1:
    cli
    push dword 0
    push dword %1
    jmp isr_common
%endmacro

%macro ISR_ERR 1
global isr%1
isr%1:
    cli
    push dword %1
    jmp isr_common
%endmacro

%macro IRQ_STUB 2
global irq%1
irq%1:
    cli
    push dword 0
    push dword %2
    jmp irq_common
%endmacro

; ---- 异常 0-31 ----
ISR_NOERR 0
ISR_NOERR 1
ISR_NOERR 2
ISR_NOERR 3
ISR_NOERR 4
ISR_NOERR 5
ISR_NOERR 6
ISR_NOERR 7
ISR_ERR   8
ISR_NOERR 9
ISR_ERR   10
ISR_ERR   11
ISR_ERR   12
ISR_ERR   13
ISR_ERR   14
ISR_NOERR 15
ISR_NOERR 16
ISR_ERR   17
ISR_NOERR 18
ISR_NOERR 19
ISR_NOERR 20
ISR_ERR   21
ISR_NOERR 22
ISR_NOERR 23
ISR_NOERR 24
ISR_NOERR 25
ISR_NOERR 26
ISR_NOERR 27
ISR_NOERR 28
ISR_ERR   29
ISR_ERR   30
ISR_NOERR 31

; ---- IRQ 0-15 => int 32-47 ----
IRQ_STUB 0,  32
IRQ_STUB 1,  33
IRQ_STUB 2,  34
IRQ_STUB 3,  35
IRQ_STUB 4,  36
IRQ_STUB 5,  37
IRQ_STUB 6,  38
IRQ_STUB 7,  39
IRQ_STUB 8,  40
IRQ_STUB 9,  41
IRQ_STUB 10, 42
IRQ_STUB 11, 43
IRQ_STUB 12, 44
IRQ_STUB 13, 45
IRQ_STUB 14, 46
IRQ_STUB 15, 47

; ---- 公共入口 ----
isr_common:
    pusha
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    push esp                    ; regs_t*
    call isr_handler
    add  esp, 4
    popa
    add  esp, 8                 ; 丢弃 int_no + err_code
    iret

irq_common:
    pusha
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    push esp
    call irq_handler
    add  esp, 4
    popa
    add  esp, 8
    iret
