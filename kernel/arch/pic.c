/* ============================================================
 * YUAOS - 8259A PIC 实现
 * 文件 : kernel/arch/pic.c
 * 说明 : 把 IRQ 0-15 重映射到中断向量 32-47
 * ============================================================ */
#include "arch/pic.h"
#include "lib/io.h"

#define ICW1_INIT   0x10
#define ICW1_ICW4   0x01
#define ICW4_8086   0x01

void pic_init(void)
{
    /* ICW1 : 开始初始化 */
    outb(PIC1_CMD,  ICW1_INIT | ICW1_ICW4); io_wait();
    outb(PIC2_CMD,  ICW1_INIT | ICW1_ICW4); io_wait();

    /* ICW2 : 向量偏移 */
    outb(PIC1_DATA, 0x20);                   io_wait(); /* 主片 -> 32 */
    outb(PIC2_DATA, 0x28);                   io_wait(); /* 从片 -> 40 */

    /* ICW3 : 主从连接 */
    outb(PIC1_DATA, 0x04);                   io_wait(); /* 从片接 IRQ2 */
    outb(PIC2_DATA, 0x02);                   io_wait(); /* 从片身份 2 */

    /* ICW4 : 8086 模式 */
    outb(PIC1_DATA, ICW4_8086);              io_wait();
    outb(PIC2_DATA, ICW4_8086);              io_wait();

    /* 屏蔽全部 */
    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);
}

void pic_send_eoi(u8 irq)
{
    if (irq >= 8) outb(PIC2_CMD, PIC_EOI);
    outb(PIC1_CMD, PIC_EOI);
}

void pic_mask(u8 irq)
{
    u16 port = (irq < 8) ? PIC1_DATA : PIC2_DATA;
    u8  bit  = (irq < 8) ? irq : (u8)(irq - 8);
    outb(port, (u8)(inb(port) |  (1u << bit)));
}

void pic_unmask(u8 irq)
{
    u16 port = (irq < 8) ? PIC1_DATA : PIC2_DATA;
    u8  bit  = (irq < 8) ? irq : (u8)(irq - 8);
    outb(port, (u8)(inb(port) & ~(1u << bit)));
}
