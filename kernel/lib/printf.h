/* ============================================================
 * YUAOS - 内核格式化输出
 * 文件 : kernel/lib/printf.h
 *
 * 支持的转换：
 *   %d %i    有符号十进制
 *   %u       无符号十进制
 *   %x %X    十六进制（小写/大写）
 *   %o       八进制
 *   %b       二进制
 *   %p       指针（0x + 8 位十六进制）
 *   %c       字符
 *   %s       字符串
 *   %%       字面量 %
 *
 * 标志：
 *   -        左对齐
 *   0        零填充（指定精度时被忽略）
 *   +        显示正号
 *   (空格)   正数前导空格
 *   #        十六进制前缀 0x / 0X
 *
 * 宽度：
 *   数字        固定宽度
 *   *           从参数读 int；负值等价于 '-' + 绝对值
 *
 * 精度：
 *   .N          最少输出 N 位数字（对 %d/%u/%x/%o/%b）；
 *               对 %s 表示最多输出 N 个字符
 *   .*          从参数读 int
 *
 * 长度修饰：
 *   l  / ll      long / long long
 *   z            size_t / ssize_t
 *   h  / hh      忽略（等同 int）
 * ============================================================ */
#ifndef YUAOS_LIB_PRINTF_H
#define YUAOS_LIB_PRINTF_H

#include "types.h"
#include <stdarg.h>

/* ---- 输出到 VGA ---- */
void kprintf (const char* fmt, ...);
void kvprintf(const char* fmt, va_list ap);

/* ---- 输出到缓冲区（类 snprintf 语义）---- */
int  ksnprintf (char* buf, size_t n, const char* fmt, ...);
int  kvsnprintf(char* buf, size_t n, const char* fmt, va_list ap);

/* ---- 裸输出 ---- */
void kputc (char c);
void kputs (const char* s);
void kputsn(const char* s, size_t n);

#endif /* YUAOS_LIB_PRINTF_H */
