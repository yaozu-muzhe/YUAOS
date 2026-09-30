/* ============================================================
 * YUAOS - 内核日志
 * 文件 : kernel/log.h
 * ============================================================ */
#ifndef YUAOS_KERNEL_LOG_H
#define YUAOS_KERNEL_LOG_H

#include "types.h"

typedef enum {
    KLOG_DEBUG = 0,
    KLOG_INFO  = 1,
    KLOG_WARN  = 2,
    KLOG_ERROR = 3,
    KLOG_FATAL = 4
} klog_level_t;

void         klog_init(void);
void         klog_set_level(klog_level_t min_level);
klog_level_t klog_get_level(void);

void klog(klog_level_t lvl, const char* fmt, ...);

/* ---- 便捷宏 ----
 * 注意 1: 宏名与左括号之间不能有空格，否则会被当成对象宏展开
 * 注意 2: ##__VA_ARGS__ 是 GCC 扩展，允许零可变参数时自动吞掉前面的逗号 */
#define KLOG_DEBUG(fmt, ...) klog(KLOG_DEBUG, fmt, ##__VA_ARGS__)
#define KLOG_INFO(fmt, ...)  klog(KLOG_INFO,  fmt, ##__VA_ARGS__)
#define KLOG_WARN(fmt, ...)  klog(KLOG_WARN,  fmt, ##__VA_ARGS__)
#define KLOG_ERROR(fmt, ...) klog(KLOG_ERROR, fmt, ##__VA_ARGS__)
#define KLOG_FATAL(fmt, ...) klog(KLOG_FATAL, fmt, ##__VA_ARGS__)

/* 断言：条件为假时输出 FATAL 并停机 */
#define KLOG_ASSERT(cond, fmt, ...) \
    do { if (!(cond)) { \
        klog(KLOG_FATAL, "assertion failed: " #cond " : " fmt, ##__VA_ARGS__); \
        for (;;) { __asm__ volatile ("cli; hlt"); } \
    } } while (0)

#endif /* YUAOS_KERNEL_LOG_H */
