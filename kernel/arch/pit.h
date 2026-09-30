/* ============================================================
 * YUAOS - 8254 PIT
 * 文件 : kernel/arch/pit.h
 * ============================================================ */
#ifndef YUAOS_ARCH_PIT_H
#define YUAOS_ARCH_PIT_H

#include "types.h"

#define PIT_BASE_FREQ 1193182

void pit_init(u32 hz);

#endif /* YUAOS_ARCH_PIT_H */
