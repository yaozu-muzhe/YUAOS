/* ============================================================
 * YUAOS - 硬件中断
 * 文件 : kernel/arch/irq.h
 * ============================================================ */
#ifndef YUAOS_ARCH_IRQ_H
#define YUAOS_ARCH_IRQ_H

#include "types.h"
#include "arch/regs.h"

typedef void (*irq_handler_t)(regs_t*);

void irq_handler(regs_t* r);
void irq_register(u8 irq, irq_handler_t fn);
void irq_unmask(u8 irq);
void irq_mask(u8 irq);

#endif /* YUAOS_ARCH_IRQ_H */
