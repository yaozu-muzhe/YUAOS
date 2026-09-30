/* ============================================================
 * YUAOS - 8254 PIT 实现
 * 文件 : kernel/arch/pit.c
 * 说明 : 通道 0，模式 3（方波），二进制计数
 * ============================================================ */
#include "arch/pit.h"
#include "lib/io.h"

#define PIT_CH0  0x40
#define PIT_CMD  0x43

void pit_init(u32 hz)
{
    if (hz == 0) hz = 100;

    u32 div = PIT_BASE_FREQ / hz;
    if (div == 0)     div = 1;
    if (div > 0xFFFF) div = 0xFFFF;

    outb(PIT_CMD, 0x36);                        /* 通道0 | 先低后高 | 模式3 | 二进制 */
    outb(PIT_CH0, (u8)( div        & 0xFF));
    outb(PIT_CH0, (u8)((div >> 8)  & 0xFF));
}
