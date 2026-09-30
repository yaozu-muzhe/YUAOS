/* ============================================================
 * YUAOS - GDT
 * 文件 : kernel/arch/gdt.h
 * ============================================================ */
#ifndef YUAOS_ARCH_GDT_H
#define YUAOS_ARCH_GDT_H

#include "types.h"

#define GDT_KERNEL_CODE 0x08
#define GDT_KERNEL_DATA 0x10
#define GDT_USER_CODE   0x18
#define GDT_USER_DATA   0x20

void gdt_init(void);

#endif /* YUAOS_ARCH_GDT_H */
