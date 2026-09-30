/* ============================================================
 * YUAOS - 帧缓冲
 * 文件 : kernel/drivers/fb.h
 * ============================================================ */
#ifndef YUAOS_DRIVERS_FB_H
#define YUAOS_DRIVERS_FB_H

#include "types.h"

#define FB_RESULT_ADDR 0x0A00u

#define RGB(r,g,b)  ((u32)(((u8)(b)) | (((u8)(g))<<8) | (((u8)(r))<<16)))

int  fb_init(void);
u32  fb_width (void);
u32  fb_height(void);
u32  fb_pitch (void);
u8*  fb_mem   (void);

void fb_put_pixel(u32 x, u32 y, u32 color);
void fb_fill_rect(u32 x, u32 y, u32 w, u32 h, u32 color);
void fb_clear(u32 color);

/* 把 framebuffer 物理页加入内核页表（vmm_init 之后调用） */
void fb_map(void);

#endif
