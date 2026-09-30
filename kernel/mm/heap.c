/* ============================================================
 * YUAOS - 内核堆实现
 * 文件 : kernel/mm/heap.c
 * 说明 :
 *   - 块头 block_t 16 字节，紧跟 data 区
 *   - 堆区 = 启动时从 PMM 申请的连续页（目标 4 MB）
 *   - 块之间物理相邻，用 next/prev 串成双向链表
 *   - 分配：首次适配，剩余空间足够则拆
 *   - 释放：标记空闲，随后与前后空闲块合并
 * ============================================================ */
#include "mm/heap.h"
#include "mm/pmm.h"
#include "lib/string.h"
#include "lib/printf.h"

/* ---- 常量 ---- */
#define HEAP_TARGET_BYTES   (4u * 1024u * 1024u)   /* 目标 4 MB */
#define HEAP_TARGET_PAGES   (HEAP_TARGET_BYTES / PAGE_SIZE)
#define HEAP_ALIGN          16u
/* 拆分后剩余 data 区不足这么多就不拆，避免产生碎片小洞 */
#define HEAP_MIN_SPLIT      32u
/* 块头大小必须是 16 字节，才能保证 data 区 16 对齐 */
_Static_assert(sizeof(void*) == 4, "heap assumes 32-bit pointers");

typedef struct block {
    u32 size;              /* data 区字节数（不含 block 头） */
    u32 in_use;            /* 0=空闲 1=占用 */
    struct block* next;
    struct block* prev;
} block_t;

_Static_assert(sizeof(block_t) == 16, "block_t must be 16 bytes");

/* ---- 全局状态 ---- */
static u32      heap_pa_start = 0;
static u32      heap_pa_end   = 0;
static block_t* heap_first    = NULL;
static block_t* heap_last     = NULL;
static u32      heap_pages    = 0;

/* ---- 工具 ---- */
static inline u32 align_up(u32 v, u32 a) { return (v + a - 1u) & ~(a - 1u); }

/* ------------------------------------------------------------
 * heap_init
 *   1. 从 PMM 循环分配连续页，直到达到目标大小或不再连续
 *   2. 把整块连续区做成一个大的空闲块
 * ------------------------------------------------------------ */
void heap_init(void)
{
    u32 first   = 0;
    u32 prev    = 0;
    u32 count   = 0;

    for (u32 i = 0; i < HEAP_TARGET_PAGES; i++) {
        u32 p = pmm_alloc_page();
        if (!p) break;

        if (i > 0 && p != prev + PAGE_SIZE) {
            /* 不再连续：退回这一页，结束 */
            pmm_free_page(p);
            break;
        }
        if (i == 0) first = p;
        prev  = p;
        count++;
    }

    if (count == 0) {
        heap_pa_start = heap_pa_end = 0;
        heap_first    = heap_last = NULL;
        heap_pages    = 0;
        return;
    }

    heap_pa_start = first;
    heap_pa_end   = first + count * PAGE_SIZE;
    heap_pages    = count;

    /* 建立唯一的初始空闲块 */
    block_t* b = (block_t*)heap_pa_start;
    b->size    = heap_pa_end - heap_pa_start - (u32)sizeof(block_t);
    b->in_use  = 0;
    b->next    = NULL;
    b->prev    = NULL;

    heap_first = b;
    heap_last  = b;
}

/* ------------------------------------------------------------
 * find_block : 首次适配
 * ------------------------------------------------------------ */
static block_t* find_block(u32 need)
{
    for (block_t* b = heap_first; b; b = b->next) {
        if (!b->in_use && b->size >= need) {
            return b;
        }
    }
    return NULL;
}

/* ------------------------------------------------------------
 * split_block : 若剩余空间足够，把 b 拆出一个新空闲块挂在后面
 * ------------------------------------------------------------ */
static void split_block(block_t* b, u32 need)
{
    if (b->size < need + sizeof(block_t) + HEAP_MIN_SPLIT) {
        return;
    }
    u8* nb_addr = (u8*)b + sizeof(block_t) + need;
    block_t* nb = (block_t*)nb_addr;

    nb->size   = b->size - need - (u32)sizeof(block_t);
    nb->in_use = 0;
    nb->next   = b->next;
    nb->prev   = b;

    if (b->next) b->next->prev = nb;
    else         heap_last     = nb;

    b->size = need;
    b->next = nb;
}

/* ------------------------------------------------------------
 * kmalloc / kzalloc / kcalloc
 * ------------------------------------------------------------ */
void* kmalloc(size_t n)
{
    if (n == 0) return NULL;

    u32 need = align_up((u32)n, HEAP_ALIGN);

    block_t* b = find_block(need);
    if (!b) return NULL;

    split_block(b, need);

    b->in_use = 1;
    return (u8*)b + sizeof(block_t);
}

void* kzalloc(size_t n)
{
    void* p = kmalloc(n);
    if (p) memset(p, 0, n);
    return p;
}

void* kcalloc(size_t nmemb, size_t size)
{
    if (nmemb == 0 || size == 0) return NULL;
    /* 检查溢出 */
    if (nmemb > (0xFFFFFFFFu / size)) return NULL;
    return kzalloc(nmemb * size);
}

/* ------------------------------------------------------------
 * kfree : 释放 + 双向合并
 * ------------------------------------------------------------ */
void kfree(void* p)
{
    if (!p) return;

    block_t* b = (block_t*)((u8*)p - sizeof(block_t));
    b->in_use = 0;

    /* 与物理相邻的下一个块合并 */
    if (b->next && !b->next->in_use) {
        block_t* n = b->next;
        b->size += (u32)sizeof(block_t) + n->size;
        b->next  = n->next;
        if (n->next) n->next->prev = b;
        else         heap_last     = b;
    }

    /* 与物理相邻的上一个块合并 */
    if (b->prev && !b->prev->in_use) {
        block_t* pv = b->prev;
        pv->size += (u32)sizeof(block_t) + b->size;
        pv->next  = b->next;
        if (b->next) b->next->prev = pv;
        else         heap_last     = pv;
    }
}

/* ------------------------------------------------------------
 * krealloc
 * ------------------------------------------------------------ */
void* krealloc(void* p, size_t n)
{
    if (!p) return kmalloc(n);
    if (n == 0) { kfree(p); return NULL; }

    block_t* b = (block_t*)((u8*)p - sizeof(block_t));
    u32 need = align_up((u32)n, HEAP_ALIGN);

    /* 现有空间已够 */
    if (b->size >= need) {
        /* 超过一半的富余可以拆出去 */
        split_block(b, need);
        return p;
    }

    /* 与下一个空闲块合并后够 */
    if (b->next && !b->next->in_use) {
        u32 combined = b->size + (u32)sizeof(block_t) + b->next->size;
        if (combined >= need) {
            block_t* nn = b->next;
            b->size = combined;
            b->next = nn->next;
            if (nn->next) nn->next->prev = b;
            else          heap_last      = b;
            split_block(b, need);
            return p;
        }
    }

    /* 老老实实搬 */
    void* np = kmalloc(n);
    if (!np) return NULL;
    memcpy(np, p, b->size);
    kfree(p);
    return np;
}

/* ------------------------------------------------------------
 * 统计
 * ------------------------------------------------------------ */
u32 heap_total(void)
{
    return heap_pa_end - heap_pa_start;
}

u32 heap_used(void)
{
    u32 used = 0;
    for (block_t* b = heap_first; b; b = b->next) {
        if (b->in_use) used += (u32)sizeof(block_t) + b->size;
    }
    return used;
}

u32 heap_free(void)
{
    return heap_total() - heap_used();
}

u32 heap_largest_free(void)
{
    u32 best = 0;
    for (block_t* b = heap_first; b; b = b->next) {
        if (!b->in_use && b->size > best) best = b->size;
    }
    return best;
}

u32 heap_block_count(void)
{
    u32 n = 0;
    for (block_t* b = heap_first; b; b = b->next) n++;
    return n;
}
