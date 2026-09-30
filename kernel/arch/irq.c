/* ============================================================
 * YUAOS - 硬件中断实现
 * 文件 : kernel/arch/irq.c
 * ============================================================ */
#include "arch/irq.h"
#include "arch/pic.h"

static irq_handler_t g_handlers[16];

void irq_register(u8 irq, irq_handler_t fn)
{
    if (irq < 16) g_handlers[irq] = fn;
}

void irq_unmask(u8 irq) { pic_unmask(irq); }
void irq_mask  (u8 irq) { pic_mask(irq);   }

void irq_handler(regs_t* r)
{
    u8 irq = (u8)(r->int_no - 32);

    if (irq < 16 && g_handlers[irq]) {
        g_handlers[irq](r);
    }

    pic_send_eoi(irq);
}
