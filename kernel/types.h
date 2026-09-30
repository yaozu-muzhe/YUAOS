/* ============================================================
 * YUAOS - 基础类型
 * 文件 : kernel/types.h
 * 说明 : 不依赖任何 libc，自定义固定宽度类型
 * ============================================================ */
#ifndef YUAOS_TYPES_H
#define YUAOS_TYPES_H

typedef unsigned char       u8;
typedef unsigned short      u16;
typedef unsigned int        u32;
typedef unsigned long long  u64;

typedef signed char         i8;
typedef signed short        i16;
typedef signed int          i32;
typedef signed long long    i64;

typedef u32                 size_t;
typedef i32                 ssize_t;

typedef u8                  bool;
#define true  1
#define false 0

#ifndef NULL
#define NULL ((void*)0)
#endif

#endif /* YUAOS_TYPES_H */
