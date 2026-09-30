/* ============================================================
 * YUAOS - 内核堆
 * 文件 : kernel/mm/heap.h
 * 说明 :
 *   - 链表堆，首次适配（first fit）
 *   - 释放时自动与前后空闲块合并
 *   - 启动时从 PMM 一次申请 4 MB 连续区，不做动态扩展
 * ============================================================ */
#ifndef YUAOS_MM_HEAP_H
#define YUAOS_MM_HEAP_H

#include "types.h"

/* ---- 生命周期 ---- */
void  heap_init(void);

/* ---- 分配 ---- */
void* kmalloc (size_t n);
void* kzalloc (size_t n);
void* kcalloc (size_t nmemb, size_t size);
void* krealloc(void* p, size_t n);
void  kfree   (void* p);

/* ---- 统计 ---- */
u32   heap_total(void);     /* 堆总字节数（含块头） */
u32   heap_used (void);     /* 已分配块（含块头）字节数 */
u32   heap_free (void);     /* 空闲块（含块头）字节数 */
u32   heap_largest_free(void); /* 最大空闲块 data 区字节数 */
u32   heap_block_count (void); /* 块节点总数 */

/* ---- 常用内存函数别名 ----
 * 内核里 memcpy/memset 本来就是自研实现，加一层 k 前缀仅为可读性；
 * 后续用户态 libyua 里会有同名符号，避免重名冲突。 */
#define kmemcpy  memcpy
#define kmemset  memset
#define kmemmove memmove
#define kmemcmp  memcmp

/* 注意：kmemcpy 等宏依赖调用方已 include "lib/string.h"。 */

#endif /* YUAOS_MM_HEAP_H */
