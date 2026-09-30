/* ============================================================
 * YUAOS - 字符串/内存函数实现
 * 文件 : kernel/lib/string.c
 * ============================================================ */
#include "lib/string.h"

void* memset(void* dst, int c, size_t n)
{
    u8* d = (u8*)dst;
    while (n--) *d++ = (u8)c;
    return dst;
}

void* memcpy(void* dst, const void* src, size_t n)
{
    u8* d = (u8*)dst;
    const u8* s = (const u8*)src;
    while (n--) *d++ = *s++;
    return dst;
}

void* memmove(void* dst, const void* src, size_t n)
{
    u8* d = (u8*)dst;
    const u8* s = (const u8*)src;

    if (d == s || n == 0) return dst;

    if (d < s) {
        while (n--) *d++ = *s++;
    } else {
        d += n;
        s += n;
        while (n--) *--d = *--s;
    }
    return dst;
}

int memcmp(const void* a, const void* b, size_t n)
{
    const u8* pa = (const u8*)a;
    const u8* pb = (const u8*)b;
    while (n--) {
        if (*pa != *pb) return (int)*pa - (int)*pb;
        pa++; pb++;
    }
    return 0;
}

size_t strlen(const char* s)
{
    size_t n = 0;
    while (s[n]) n++;
    return n;
}

char* strcpy(char* dst, const char* src)
{
    char* d = dst;
    while ((*d++ = *src++)) { /* empty */ }
    return dst;
}

char* strncpy(char* dst, const char* src, size_t n)
{
    char* d = dst;
    while (n && *src) { *d++ = *src++; n--; }
    while (n--) *d++ = 0;
    return dst;
}

int strcmp(const char* a, const char* b)
{
    while (*a && *a == *b) { a++; b++; }
    return (int)(u8)*a - (int)(u8)*b;
}

int strncmp(const char* a, const char* b, size_t n)
{
    while (n && *a && *a == *b) { a++; b++; n--; }
    if (n == 0) return 0;
    return (int)(u8)*a - (int)(u8)*b;
}

char* strcat(char* dst, const char* src)
{
    char* d = dst + strlen(dst);
    while ((*d++ = *src++)) { /* empty */ }
    return dst;
}

char* strchr(const char* s, int c)
{
    while (*s) {
        if (*s == (char)c) return (char*)s;
        s++;
    }
    return (c == 0) ? (char*)s : NULL;
}

char* strrchr(const char* s, int c)
{
    const char* last = NULL;
    while (*s) {
        if (*s == (char)c) last = s;
        s++;
    }
    if (c == 0) return (char*)s;
    return (char*)last;
}
