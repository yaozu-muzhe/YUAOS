/* ============================================================
 * YUAOS - 物理页管理器实现
 * 文件 : kernel/mm/pmm.c
 * 说明 : 位图 1 bit / 4KB 页；位图自身放在 1MB 以上首个可用区
 * ============================================================ */
#include "mm/pmm.h"
#include "mm/e820.h"
#include "lib/string.h"
#include "lib/printf.h"

/* 4GB / 4KB = 1M 页 → 位图 128 KB */
#define PMM_MAX_PAGES    (1024u * 1024u)
#define PMM_BITMAP_BYTES (PMM_MAX_PAGES / 8u)

extern u8 _kernel_end[];

static u8*  pmm_bitmap     = NULL;
static u32  pmm_bitmap_pa  = 0;
static u32  pmm_bitmap_len = 0;
static u32  pmm_total      = 0;
static u32  pmm_free_cnt   = 0;

static inline void bm_set  (u32 i) { pmm_bitmap[i >> 3] |=  (u8)(1u << (i & 7)); }
static inline void bm_clear(u32 i) { pmm_bitmap[i >> 3] &= (u8)~(1u << (i & 7)); }
static inline u32  bm_test (u32 i) { return (pmm_bitmap[i >> 3] >> (i & 7)) & 1u; }

static void bm_mark_used(u32 page)
{
    if (page >= pmm_total) return;
    if (!bm_test(page)) {
        bm_set(page);
        if (pmm_free_cnt) pmm_free_cnt--;
    }
}

static void bm_mark_free(u32 page)
{
    if (page >= pmm_total) return;
    if (bm_test(page)) {
        bm_clear(page);
        pmm_free_cnt++;
    }
}

/* 在 E820 中找第一块 1MB 以上、足够容纳位图的 usable 区 */
static u32 find_bitmap_region(u32 bytes)
{
    const e820_map_t* map = e820_get_map();
    for (u32 i = 0; i < map->count && i < E820_MAX_ENTRIES; i++) {
        const e820_entry_t* e = &map->entries[i];
        if (e->type != E820_TYPE_USABLE) continue;
        if (e->base_hi != 0) continue;          /* 忽略 4G 以上 */
        if (e->base_lo < 0x100000) continue;    /* 从 1MB 起找 */
        if (e->len_lo  < bytes) continue;
        return e->base_lo;
    }
    return 0;
}

void pmm_init(void)
{
    const e820_map_t* map = e820_get_map();

    /* 1. 总页数 */
    u32 max = e820_max_address();
    pmm_total = max / PAGE_SIZE;
    if (pmm_total > PMM_MAX_PAGES) pmm_total = PMM_MAX_PAGES;

    pmm_bitmap_len = (pmm_total + 7u) / 8u;
    if (pmm_bitmap_len > PMM_BITMAP_BYTES) pmm_bitmap_len = PMM_BITMAP_BYTES;

    /* 2. 找位图落地点 */
    pmm_bitmap_pa = find_bitmap_region(PAGE_ALIGN_UP(pmm_bitmap_len));
    if (pmm_bitmap_pa == 0) {
        kprintf("PMM: no room for bitmap!\n");
        return;
    }
    pmm_bitmap = (u8*)pmm_bitmap_pa;    /* 无分页，物理即虚拟 */

    /* 3. 默认全占 */
    memset(pmm_bitmap, 0xFF, pmm_bitmap_len);
    pmm_free_cnt = 0;

    /* 4. E820 usable → 空闲 */
    for (u32 i = 0; i < map->count && i < E820_MAX_ENTRIES; i++) {
        const e820_entry_t* e = &map->entries[i];
        if (e->type != E820_TYPE_USABLE) continue;
        if (e->base_hi != 0) continue;

        u32 start = e->base_lo / PAGE_SIZE;
        u32 end   = (e->base_lo + e->len_lo) / PAGE_SIZE;
        if (end > pmm_total) end = pmm_total;
        for (u32 p = start; p < end; p++) bm_mark_free(p);
    }

    /* 5. 保留低 1MB */
    for (u32 p = 0; p < (0x100000u / PAGE_SIZE); p++) bm_mark_used(p);

    /* 6. 保留内核 */
    u32 kstart = 0x10000u / PAGE_SIZE;
    u32 kend   = PAGE_ALIGN_UP((u32)_kernel_end) / PAGE_SIZE;
    for (u32 p = kstart; p < kend; p++) bm_mark_used(p);

    /* 7. 保留位图自身 */
    u32 bstart = pmm_bitmap_pa / PAGE_SIZE;
    u32 bend   = PAGE_ALIGN_UP(pmm_bitmap_pa + pmm_bitmap_len) / PAGE_SIZE;
    for (u32 p = bstart; p < bend; p++) bm_mark_used(p);
}

u32 pmm_alloc_page(void)
{
    /* 简单线性扫描；后续可换 bitmap_find_free 优化 */
    for (u32 i = 0; i < pmm_total; i++) {
        if (!bm_test(i)) {
            bm_set(i);
            pmm_free_cnt--;
            return i * PAGE_SIZE;
        }
    }
    return 0;
}

void pmm_free_page(u32 addr)
{
    u32 p = addr / PAGE_SIZE;
    if (p < pmm_total && bm_test(p)) {
        bm_clear(p);
        pmm_free_cnt++;
    }
}

u32 pmm_total_pages (void) { return pmm_total; }
u32 pmm_free_pages  (void) { return pmm_free_cnt; }
u32 pmm_bitmap_addr (void) { return pmm_bitmap_pa; }
u32 pmm_bitmap_size (void) { return pmm_bitmap_len; }
