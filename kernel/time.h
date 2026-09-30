/* ============================================================
 * YUAOS - 全局时间
 * 文件 : kernel/time.h
 * 说明 : 由 PIT (100 Hz) 中断驱动；定义在 main.c 中
 * ============================================================ */
#ifndef YUAOS_KERNEL_TIME_H
#define YUAOS_KERNEL_TIME_H

#include "types.h"

/* 系统 tick 计数器（1 tick = 10 ms） */
extern volatile u64 g_kernel_ticks;

static inline u64 kernel_ticks(void)   { return g_kernel_ticks; }
static inline u32 kernel_seconds(void) { return (u32)(g_kernel_ticks / 100); }
static inline u32 kernel_centis(void)  { return (u32)(g_kernel_ticks % 100); }

#endif /* YUAOS_KERNEL_TIME_H */
