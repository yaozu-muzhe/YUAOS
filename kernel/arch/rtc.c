/* ============================================================
 * YUAOS - CMOS RTC 实现
 * 文件 : kernel/arch/rtc.c
 * 说明 : 端口 0x70/0x71；等 UIP 清零后读，支持 BCD 与 12 小时制
 * ============================================================ */
#include "arch/rtc.h"
#include "lib/io.h"

static u8 cmos_read(u8 reg)
{
    outb(0x70, reg);
    return inb(0x71);
}

static int cmos_updating(void)
{
    return (cmos_read(0x0A) & 0x80) != 0;
}

static u8 bcd2bin(u8 v)
{
    return (u8)((v & 0x0F) + ((v >> 4) * 10));
}

void rtc_read(rtc_time_t* t)
{
    if (!t) return;

    while (cmos_updating()) { /* spin */ }

    u8 s  = cmos_read(0x00);
    u8 m  = cmos_read(0x02);
    u8 h  = cmos_read(0x04);
    u8 d  = cmos_read(0x07);
    u8 mo = cmos_read(0x08);
    u8 y  = cmos_read(0x09);
    u8 st = cmos_read(0x0B);

    int is_bcd = !(st & 0x04);
    int is_12h = !(st & 0x02);
    int pm     = 0;

    if (is_bcd) {
        if (is_12h) pm = (h & 0x80) != 0;
        h &= 0x7F;
        s  = bcd2bin(s);
        m  = bcd2bin(m);
        h  = bcd2bin(h);
        d  = bcd2bin(d);
        mo = bcd2bin(mo);
        y  = bcd2bin(y);
    } else {
        if (is_12h) pm = (h & 0x80) != 0;
        h &= 0x7F;
    }

    if (is_12h) {
        if (pm && h < 12) h += 12;
        if (!pm && h == 12) h = 0;
    }

    t->second = s;
    t->minute = m;
    t->hour   = h;
    t->day    = d;
    t->month  = mo;
    t->year   = (u16)(2000 + y);
}
