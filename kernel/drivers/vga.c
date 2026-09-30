/* ============================================================
 * YUAOS - 文本渲染实现
 * 文件 : kernel/drivers/vga.c
 * ============================================================ */
#include "drivers/vga.h"
#include "drivers/fb.h"
#include "drivers/font_data.h"
#include "lib/string.h"

#define CW 8
#define CH 16

static u32 g_cols = 0;
static u32 g_rows = 0;
static u32 g_row  = 0;
static u32 g_col  = 0;

static u32 g_fg = VGA_WHITE;
static u32 g_bg = VGA_BLACK;

/* ------------------------------------------------------------ */
static void draw_glyph(u32 col, u32 row, const u8* glyph)
{
    u32 px = col * CW;
    u32 py = row * CH;

    for (u32 j = 0; j < 16; j++) {
        u8 bits = glyph[j];
        for (u32 i = 0; i < 8; i++) {
            u32 color = (bits & (0x80 >> i)) ? g_fg : g_bg;
            fb_put_pixel(px + i, py + j, color);
        }
    }
}

static const u8* glyph_for(u8 c)
{
    if (c < FONT_DATA_FIRST || c > FONT_DATA_LAST) {
        c = '?';
    }
    return g_font_data_8x16 + (c - FONT_DATA_FIRST) * 16;
}

/* ------------------------------------------------------------
 * 上滚一行；然后把光标放回最后一行
 * ------------------------------------------------------------ */
static void scroll_up(void)
{
    u32 pitch = fb_pitch();
    u32 h     = fb_height();
    u32 w     = fb_width();
    u8* fb    = fb_mem();

    u32 row_bytes = w * 4;
    for (u32 y = CH; y < h; y++) {
        memcpy(fb + (y - CH) * pitch, fb + y * pitch, row_bytes);
    }
    fb_fill_rect(0, h - CH, w, CH, g_bg);

    /* ★ 关键修复：把光标拉回最后一行 */
    if (g_rows > 0) {
        g_row = g_rows - 1;
    } else {
        g_row = 0;
    }
}

/* ------------------------------------------------------------ */
void vga_init(void)
{
    fb_init();
    g_cols = fb_width()  / CW;
    g_rows = fb_height() / CH;
    g_row  = 0;
    g_col  = 0;
    vga_clear();
}

void vga_clear(void)
{
    fb_clear(g_bg);
    g_row = 0;
    g_col = 0;
}

void vga_set_color(u32 fg, u32 bg) { g_fg = fg; g_bg = bg; }

void vga_putc(char c)
{
    u8 b = (u8)c;

    if (b == '\n') {
        g_col = 0;
        g_row++;
    }
    else if (b == '\r') {
        g_col = 0;
    }
    else if (b == '\t') {
        u32 next = (g_col + 4) & ~3u;
        while (g_col < next && g_col < g_cols) {
            draw_glyph(g_col, g_row, glyph_for(' '));
            g_col++;
        }
    }
    else if (b == '\b') {
        if (g_col > 0) {
            g_col--;
            draw_glyph(g_col, g_row, glyph_for(' '));
        }
    }
    else {
        if (g_col >= g_cols) {
            g_col = 0;
            g_row++;
        }
        if (g_row >= g_rows) {
            scroll_up();
        }
        draw_glyph(g_col, g_row, glyph_for(b));
        g_col++;
    }

    if (g_row >= g_rows) {
        scroll_up();
    }
}

void vga_write(const char* s) { while (*s) vga_putc(*s++); }

u32 vga_get_row(void) { return g_row; }
u32 vga_get_col(void) { return g_col; }

void vga_set_cursor(u32 row, u32 col)
{
    if (row < g_rows) g_row = row;
    if (col < g_cols) g_col = col;
}
