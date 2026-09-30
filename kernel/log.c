/* ============================================================
 * YUAOS - 内核日志实现
 * 文件 : kernel/log.c
 * ============================================================ */
#include "log.h"
#include "time.h"
#include "drivers/vga.h"
#include "lib/printf.h"
#include <stdarg.h>

static klog_level_t g_min_level = KLOG_DEBUG;

static const char* level_tag(klog_level_t l)
{
    switch (l) {
    case KLOG_DEBUG: return "DBG";
    case KLOG_INFO:  return "INF";
    case KLOG_WARN:  return "WRN";
    case KLOG_ERROR: return "ERR";
    case KLOG_FATAL: return "FTL";
    }
    return "???";
}

/* 返回 24 位颜色，类型为 u32 */
static u32 level_fg(klog_level_t l)
{
    switch (l) {
    case KLOG_DEBUG: return VGA_DARK_GREY;
    case KLOG_INFO:  return VGA_LIGHT_GREEN;
    case KLOG_WARN:  return VGA_YELLOW;
    case KLOG_ERROR: return VGA_LIGHT_RED;
    case KLOG_FATAL: return VGA_LIGHT_MAGENTA;
    }
    return VGA_WHITE;
}

void klog_init(void)
{
    g_min_level = KLOG_DEBUG;
}

void klog_set_level(klog_level_t lvl)
{
    g_min_level = lvl;
}

klog_level_t klog_get_level(void)
{
    return g_min_level;
}

void klog(klog_level_t lvl, const char* fmt, ...)
{
    if (lvl < g_min_level) return;

    u64 t = kernel_ticks();

    vga_set_color(VGA_DARK_GREY, VGA_BLACK);
    kprintf("[%5u.%02u] ", (u32)(t / 100), (u32)(t % 100));

    vga_set_color(level_fg(lvl), VGA_BLACK);
    kprintf("[%s] ", level_tag(lvl));

    vga_set_color(VGA_WHITE, VGA_BLACK);
    va_list ap;
    va_start(ap, fmt);
    kvprintf(fmt, ap);
    va_end(ap);

    vga_putc('\n');
}
