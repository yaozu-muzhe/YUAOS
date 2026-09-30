/* ============================================================
 * YUAOS - 8259A PIC
 * 文件 : kernel/arch/pic.h
 * ============================================================ */
#ifndef YUAOS_ARCH_PIC_H
#define YUAOS_ARCH_PIC_H

#include "types.h"

#define PIC1_CMD    0x20
#define PIC1_DATA   0x21
#define PIC2_CMD    0xA0
#define PIC2_DATA   0xA1

#define PIC_EOI     0x20

void pic_init(void);
void pic_send_eoi(u8 irq);
void pic_mask(u8 irq);
void pic_unmask(u8 irq);

#endif /* YUAOS_ARCH_PIC_H */
