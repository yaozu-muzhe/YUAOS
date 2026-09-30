/* ============================================================
 * YUAOS - 文本渲染（唯一渲染器）
 * 文件 : kernel/drivers/vga.h
 * 说明 : 直接使用 fb.c 和 font_data.h，无中间层、无 UTF-8 状态机。
 *        只接受 ASCII 0x20-0x7E，其它字节一律显示 '?'。
 * ============================================================ */
#ifndef YUAOS_DRIVERS_VGA_H
#define YUAOS_DRIVERS_VGA_H

#include "types.h"

#define VGA_BLACK          0x000000u
#define VGA_BLUE           0x0000AAu
#define VGA_GREEN          0x00AA00u
#define VGA_CYAN           0x00AAAAu
#define VGA_RED            0xAA0000u
#define VGA_MAGENTA        0xAA00AAu
#define VGA_BROWN          0xAA5500u
#define VGA_LIGHT_GREY     0xAAAAAAu
#define VGA_DARK_GREY      0x555555u
#define VGA_LIGHT_BLUE     0x5555FFu
#define VGA_LIGHT_GREEN    0x55FF55u
#define VGA_LIGHT_CYAN     0x55FFFFu
#define VGA_LIGHT_RED      0xFF5555u
#define VGA_LIGHT_MAGENTA  0xFF55FFu
#define VGA_YELLOW         0xFFFF55u
#define VGA_WHITE          0xFFFFFFu

void vga_init(void);
void vga_clear(void);
void vga_putc(char c);
void vga_write(const char* s);
void vga_set_color(u32 fg, u32 bg);

u32  vga_get_row(void);
u32  vga_get_col(void);
void vga_set_cursor(u32 row, u32 col);

#endif
