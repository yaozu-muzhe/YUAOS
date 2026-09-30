/* ============================================================
 * YUAOS - 异常分发
 * 文件 : kernel/arch/isr.h
 * ============================================================ */
#ifndef YUAOS_ARCH_ISR_H
#define YUAOS_ARCH_ISR_H

#include "types.h"
#include "arch/regs.h"

void        isr_handler(regs_t* r);
const char* exception_name(u32 n);

#endif /* YUAOS_ARCH_ISR_H */
