/* ============================================================
 * YUAOS - 物理页管理器（位图）
 * 文件 : kernel/mm/pmm.h
 * ============================================================ */
#ifndef YUAOS_MM_PMM_H
#define YUAOS_MM_PMM_H

#include "types.h"

#define PAGE_SIZE  4096
#define PAGE_SHIFT 12
#define PAGE_ALIGN_UP(x)   (((x) + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1))
#define PAGE_ALIGN_DOWN(x) ((x) & ~(PAGE_SIZE - 1))

void pmm_init(void);

/* 返回页帧物理基址；失败返回 0 */
u32  pmm_alloc_page(void);
void pmm_free_page(u32 addr);

u32  pmm_total_pages(void);
u32  pmm_free_pages(void);
u32  pmm_bitmap_addr(void);
u32  pmm_bitmap_size(void);

#endif /* YUAOS_MM_PMM_H */
