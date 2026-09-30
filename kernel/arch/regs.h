/* ============================================================
 * YUAOS - 中断栈帧
 * 文件 : kernel/arch/regs.h
 * 说明 : isr_common / irq_common 在栈上布局的精确映射；
 *        字段顺序与 pusha + CPU 自动压栈顺序严格一致。
 * ============================================================ */
#ifndef YUAOS_ARCH_REGS_H
#define YUAOS_ARCH_REGS_H

#include "types.h"

typedef struct regs {
    /* ---- 由 pusha 压入（低地址 → 高地址）---- */
    u32 edi, esi, ebp, esp_dummy, ebx, edx, ecx, eax;
    /* ---- 由汇编桩 / CPU 压入 ---- */
    u32 int_no, err_code;
    u32 eip, cs, eflags;
    /* ---- CPU 在特权级切换时压入（内核态触发时无意义）---- */
    u32 useresp, ss;
} regs_t;

#endif /* YUAOS_ARCH_REGS_H */
