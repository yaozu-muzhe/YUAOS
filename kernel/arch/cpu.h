/* ============================================================
 * YUAOS - CPU 基础操作
 * 文件 : kernel/arch/cpu.h
 * ============================================================ */
#ifndef YUAOS_ARCH_CPU_H
#define YUAOS_ARCH_CPU_H

#include "types.h"

static inline void cpu_cli(void) { __asm__ volatile ("cli" ::: "memory"); }
static inline void cpu_sti(void) { __asm__ volatile ("sti" ::: "memory"); }
static inline void cpu_hlt(void) { __asm__ volatile ("hlt" ::: "memory"); }

/* 停在此处，直到永远 */
static inline void cpu_halt_forever(void)
{
    for (;;) {
        __asm__ volatile ("cli; hlt");
    }
}

#endif /* YUAOS_ARCH_CPU_H */
