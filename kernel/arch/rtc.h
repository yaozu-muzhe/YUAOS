/* ============================================================
 * YUAOS - CMOS RTC
 * 文件 : kernel/arch/rtc.h
 * ============================================================ */
#ifndef YUAOS_ARCH_RTC_H
#define YUAOS_ARCH_RTC_H

#include "types.h"

typedef struct {
    u8  second, minute, hour;
    u8  day, month;
    u16 year;
} rtc_time_t;

void rtc_read(rtc_time_t* t);

#endif
