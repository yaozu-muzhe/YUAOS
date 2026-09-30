/* ============================================================
 * YUAOS - 内核格式化输出实现
 * 文件 : kernel/lib/printf.c
 * 说明 : 单一核心 format_core，多后端（VGA / 缓冲区）
 * ============================================================ */
#include "lib/printf.h"
#include "drivers/vga.h"

/* ============================================================
 * 输出后端抽象
 * ============================================================ */
typedef void (*out_fn_t)(void* ctx, char c);

typedef struct {
    char* buf;
    size_t cap;
    size_t len;    /* 本应写入的总字符数（不管溢出） */
} buf_ctx_t;

static void vga_out(void* ctx, char c)
{
    (void)ctx;
    vga_putc(c);
}

static void buf_out(void* ctx, char c)
{
    buf_ctx_t* b = (buf_ctx_t*)ctx;
    if (b->buf && b->len + 1 < b->cap) {
        b->buf[b->len] = c;
    }
    b->len++;
}

/* ============================================================
 * 工具
 * ============================================================ */
static int out_pad(out_fn_t out, void* ctx, char c, int n)
{
    int i = 0;
    for (; i < n; i++) out(ctx, c);
    return i;
}

/* ------------------------------------------------------------
 * format_number : 输出一个整数
 *   value  : 已取绝对值的数值
 *   base   : 进制
 *   upper  : 十六进制字母大写
 *   prefix : 前缀字符串（如 "-"、"+"、"0x"），可为 NULL
 *   width  : 最小总宽度
 *   zero   : 零填充（仅当 prec < 0 时生效）
 *   left   : 左对齐
 *   prec   : 最少位数；< 0 表示未指定
 * 返回：写入字符数
 * ------------------------------------------------------------ */
static int format_number(out_fn_t out, void* ctx,
                         u64 value, int base, bool upper,
                         const char* prefix,
                         int width, bool zero, bool left, int prec)
{
    char digits[66];
    int  nd = 0;
    const char* dc = upper ? "0123456789ABCDEF" : "0123456789abcdef";

    if (value == 0) {
        if (prec != 0) digits[nd++] = '0';
    } else {
        while (value) {
            digits[nd++] = dc[value % base];
            value /= base;
        }
    }
    while (nd < prec) digits[nd++] = '0';

    int plen = 0;
    if (prefix) while (prefix[plen]) plen++;

    int total = plen + nd;
    int pad   = (width > total) ? width - total : 0;
    int written = 0;

    if (left) {
        for (int i = 0; i < plen; i++) { out(ctx, prefix[i]); written++; }
        while (nd--) { out(ctx, digits[nd]); written++; }
        written += out_pad(out, ctx, ' ', pad);
    } else if (zero && prec < 0) {
        for (int i = 0; i < plen; i++) { out(ctx, prefix[i]); written++; }
        written += out_pad(out, ctx, '0', pad);
        while (nd--) { out(ctx, digits[nd]); written++; }
    } else {
        written += out_pad(out, ctx, ' ', pad);
        for (int i = 0; i < plen; i++) { out(ctx, prefix[i]); written++; }
        while (nd--) { out(ctx, digits[nd]); written++; }
    }
    return written;
}

/* ============================================================
 * 格式描述符
 * ============================================================ */
typedef struct {
    bool left;
    bool zero;
    bool plus;
    bool space;
    bool alt;
    int  width;
    int  prec;      /* -1 = 未指定 */
    int  length;    /* 0=int, 1=long, 2=long long, 3=size_t */
} fmt_t;

/* ============================================================
 * 核心引擎
 * ============================================================ */
static int format_core(out_fn_t out, void* ctx, const char* fmt, va_list ap)
{
    int written = 0;

    while (*fmt) {
        if (*fmt != '%') {
            out(ctx, *fmt++);
            written++;
            continue;
        }
        fmt++;                              /* 跳过 '%' */

        /* ---- 标志 ---- */
        fmt_t f = { false, false, false, false, false, 0, -1, 0 };
        for (;; fmt++) {
            if      (*fmt == '-') f.left  = true;
            else if (*fmt == '0') f.zero  = true;
            else if (*fmt == '+') f.plus  = true;
            else if (*fmt == ' ') f.space = true;
            else if (*fmt == '#') f.alt   = true;
            else break;
        }

        /* ---- 宽度 ---- */
        if (*fmt == '*') {
            int w = va_arg(ap, int);
            if (w < 0) { f.left = true; w = -w; }
            f.width = w;
            fmt++;
        } else {
            while (*fmt >= '0' && *fmt <= '9') {
                f.width = f.width * 10 + (*fmt - '0');
                fmt++;
            }
        }

        /* ---- 精度 ---- */
        if (*fmt == '.') {
            fmt++;
            if (*fmt == '*') {
                f.prec = va_arg(ap, int);
                if (f.prec < 0) f.prec = -1;
                fmt++;
            } else {
                f.prec = 0;
                while (*fmt >= '0' && *fmt <= '9') {
                    f.prec = f.prec * 10 + (*fmt - '0');
                    fmt++;
                }
            }
        }

        /* ---- 长度修饰 ---- */
        if (*fmt == 'l') {
            fmt++;
            if (*fmt == 'l') { f.length = 2; fmt++; }
            else              f.length = 1;
        } else if (*fmt == 'z') {
            f.length = 3; fmt++;
        } else if (*fmt == 'h') {
            fmt++;
            if (*fmt == 'h') fmt++;
            f.length = 0;
        }

        char spec = *fmt;
        if (!spec) break;

        switch (spec) {

        /* ---------- 有符号十进制 ---------- */
        case 'd': case 'i': {
            i64 v;
            if      (f.length == 2) v = va_arg(ap, long long);
            else if (f.length == 1) v = va_arg(ap, long);
            else if (f.length == 3) v = (i64)va_arg(ap, ssize_t);
            else                    v = va_arg(ap, int);

            bool neg = (v < 0);
            u64  mag = neg ? ((u64)(-(v + 1)) + 1u) : (u64)v;

            char pfx[3]; int plen = 0;
            if      (neg)        pfx[plen++] = '-';
            else if (f.plus)     pfx[plen++] = '+';
            else if (f.space)    pfx[plen++] = ' ';
            pfx[plen] = 0;

            written += format_number(out, ctx, mag, 10, false, pfx,
                                     f.width, f.zero, f.left, f.prec);
            break;
        }

        /* ---------- 无符号十进制 ---------- */
        case 'u': {
            u64 v;
            if      (f.length == 2) v = va_arg(ap, unsigned long long);
            else if (f.length == 1) v = va_arg(ap, unsigned long);
            else if (f.length == 3) v = (u64)va_arg(ap, size_t);
            else                    v = va_arg(ap, unsigned int);

            written += format_number(out, ctx, v, 10, false, NULL,
                                     f.width, f.zero, f.left, f.prec);
            break;
        }

        /* ---------- 十六进制 ---------- */
        case 'x': case 'X': {
            u64 v;
            if      (f.length == 2) v = va_arg(ap, unsigned long long);
            else if (f.length == 1) v = va_arg(ap, unsigned long);
            else if (f.length == 3) v = (u64)va_arg(ap, size_t);
            else                    v = va_arg(ap, unsigned int);

            char pfx[3]; int plen = 0;
            if (f.alt && v != 0) {
                pfx[plen++] = '0';
                pfx[plen++] = (spec == 'X') ? 'X' : 'x';
            }
            pfx[plen] = 0;

            written += format_number(out, ctx, v, 16, spec == 'X', pfx,
                                     f.width, f.zero, f.left, f.prec);
            break;
        }

        /* ---------- 八进制 ---------- */
        case 'o': {
            u64 v;
            if      (f.length == 2) v = va_arg(ap, unsigned long long);
            else if (f.length == 1) v = va_arg(ap, unsigned long);
            else if (f.length == 3) v = (u64)va_arg(ap, size_t);
            else                    v = va_arg(ap, unsigned int);

            const char* pfx = (f.alt && v != 0) ? "0" : NULL;
            written += format_number(out, ctx, v, 8, false, pfx,
                                     f.width, f.zero, f.left, f.prec);
            break;
        }

        /* ---------- 二进制 ---------- */
        case 'b': {
            u64 v;
            if      (f.length == 2) v = va_arg(ap, unsigned long long);
            else if (f.length == 1) v = va_arg(ap, unsigned long);
            else if (f.length == 3) v = (u64)va_arg(ap, size_t);
            else                    v = va_arg(ap, unsigned int);

            written += format_number(out, ctx, v, 2, false, NULL,
                                     f.width, f.zero, f.left, f.prec);
            break;
        }

        /* ---------- 指针 ---------- */
        case 'p': {
            u32 v = (u32)va_arg(ap, void*);
            out(ctx, '0'); out(ctx, 'x'); written += 2;
            written += format_number(out, ctx, v, 16, false, NULL,
                                     8, true, false, -1);
            break;
        }

        /* ---------- 字符 ---------- */
        case 'c': {
            char c = (char)va_arg(ap, int);
            if (!f.left && f.width > 1) written += out_pad(out, ctx, ' ', f.width - 1);
            out(ctx, c); written++;
            if (f.left && f.width > 1)  written += out_pad(out, ctx, ' ', f.width - 1);
            break;
        }

        /* ---------- 字符串 ---------- */
        case 's': {
            const char* s = va_arg(ap, const char*);
            if (!s) s = "(null)";
            int len = 0;
            while (s[len]) len++;
            if (f.prec >= 0 && len > f.prec) len = f.prec;
            int pad = (f.width > len) ? f.width - len : 0;
            if (!f.left) written += out_pad(out, ctx, ' ', pad);
            for (int i = 0; i < len; i++) { out(ctx, s[i]); written++; }
            if (f.left)  written += out_pad(out, ctx, ' ', pad);
            break;
        }

        /* ---------- 百分号 ---------- */
        case '%':
            out(ctx, '%'); written++;
            break;

        default:
            out(ctx, '%'); out(ctx, spec); written += 2;
            break;
        }
        fmt++;
    }
    return written;
}

/* ============================================================
 * 公共 API
 * ============================================================ */
void kprintf(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    format_core(vga_out, NULL, fmt, ap);
    va_end(ap);
}

void kvprintf(const char* fmt, va_list ap)
{
    format_core(vga_out, NULL, fmt, ap);
}

int kvsnprintf(char* buf, size_t n, const char* fmt, va_list ap)
{
    buf_ctx_t c = { buf, n, 0 };
    int r = format_core(buf_out, &c, fmt, ap);

    if (n > 0) {
        size_t term = (c.len < n) ? c.len : (n - 1);
        buf[term] = 0;
    }
    return r;
}

int ksnprintf(char* buf, size_t n, const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int r = kvsnprintf(buf, n, fmt, ap);
    va_end(ap);
    return r;
}

void kputc (char c)                  { vga_putc(c); }
void kputs (const char* s)           { while (*s) vga_putc(*s++); }
void kputsn(const char* s, size_t n) { for (size_t i = 0; i < n && s[i]; i++) vga_putc(s[i]); }
