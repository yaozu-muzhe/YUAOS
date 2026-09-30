/* ============================================================
 * YUAOS - 帧缓冲实现
 * 文件 : kernel/drivers/fb.c
 * ============================================================ */
#include "drivers/fb.h"
#include "arch/paging.h"
#include "mm/pmm.h"
#include "lib/printf.h"

static u8*  g_fb       = NULL;
static u32  g_fb_w     = 0;
static u32  g_fb_h     = 0;
static u32  g_fb_pitch = 0;
static u8   g_fb_bpp   = 0;

int fb_init(void)
{
    volatile const u8* p = (volatile const u8*)FB_RESULT_ADDR;

    u32 addr  = *(volatile const u32*)(p + 0);
    u16 w     = *(volatile const u16*)(p + 4);
    u16 h     = *(volatile const u16*)(p + 6);
    u16 pitch = *(volatile const u16*)(p + 8);
    u8  bpp   = *(volatile const u8*)(p + 10);
    u8  ok    = *(volatile const u8*)(p + 11);

    if (!ok || bpp != 32 || w == 0 || h == 0) return -1;

    g_fb       = (u8*)addr;
    g_fb_w     = w;
    g_fb_h     = h;
    g_fb_pitch = pitch;
    g_fb_bpp   = bpp;

    /* 诊断：把 VBE 报告的参数打出来 */
    kprintf("fb : addr=0x%X w=%u h=%u pitch=%u bpp=%u\n",
            addr, (u32)w, (u32)h, (u32)pitch, (u32)bpp);

    return 0;
}

u32 fb_width (void) { return g_fb_w; }
u32 fb_height(void) { return g_fb_h; }
u32 fb_pitch (void) { return g_fb_pitch; }
u8* fb_mem   (void) { return g_fb; }

void fb_put_pixel(u32 x, u32 y, u32 color)
{
    if (x >= g_fb_w || y >= g_fb_h) return;
    *(volatile u32*)(g_fb + y * g_fb_pitch + x * 4) = color;
}

void fb_fill_rect(u32 x, u32 y, u32 w, u32 h, u32 color)
{
    if (!g_fb) return;
    if (x >= g_fb_w || y >= g_fb_h) return;
    if (x + w > g_fb_w) w = g_fb_w - x;
    if (y + h > g_fb_h) h = g_fb_h - y;

    for (u32 j = 0; j < h; j++) {
        volatile u32* row = (volatile u32*)(g_fb + (y + j) * g_fb_pitch + x * 4);
        for (u32 i = 0; i < w; i++) row[i] = color;
    }
}

void fb_clear(u32 color)
{
    fb_fill_rect(0, 0, g_fb_w, g_fb_h, color);
}

/* ============================================================
 * 把 framebuffer 所在物理页加入内核页表
 * VBE LFB 通常位于 0xE0000000 以上，超出默认 128MB 恒等映射
 * ============================================================ */
void fb_map(void)
{
    if (!g_fb) return;

    u32 base = (u32)g_fb & 0xFFE00000u;   /* 2 MB 对齐起点 */
    u32 size = 2u * 1024u * 1024u;        /* 映射 2 MB */

    for (u32 off = 0; off < size; off += PAGE_SIZE) {
        u32 addr = base + off;
        if (vmm_get_phys(addr) == 0) {
            vmm_map_page(addr, addr, PTE_RW);
        }
    }
}
