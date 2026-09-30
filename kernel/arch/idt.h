/* ============================================================
 * YUAOS - IDT
 * 文件 : kernel/arch/idt.h
 * ============================================================ */
#ifndef YUAOS_ARCH_IDT_H
#define YUAOS_ARCH_IDT_H

#include "types.h"

void idt_init(void);
void idt_set_gate(u8 num, u32 base, u16 sel, u8 flags);

#endif /* YUAOS_ARCH_IDT_H */
