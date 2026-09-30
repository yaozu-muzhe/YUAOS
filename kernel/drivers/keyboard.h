/* ============================================================
 * YUAOS - PS/2 键盘驱动
 * 文件 : kernel/drivers/keyboard.h
 * ============================================================ */
#ifndef YUAOS_DRIVERS_KEYBOARD_H
#define YUAOS_DRIVERS_KEYBOARD_H

#include "types.h"
#include "arch/regs.h"

/* 特殊键编码（>0x7F，与普通 ASCII 区分） */
#define KB_KEY_UP     0x80
#define KB_KEY_DOWN   0x81
#define KB_KEY_LEFT   0x82
#define KB_KEY_RIGHT  0x83
#define KB_KEY_HOME   0x84
#define KB_KEY_END    0x85
#define KB_KEY_DELETE 0x86

void kb_init(void);
void kb_irq_handler(regs_t* r);

int  kb_try_getchar(void);   /* 无键返回 -1 */
int  kb_wait_getchar(void);  /* 阻塞直到有键 */

#endif
