/* ============================================================
 * YUAOS - 虚拟内存 / 分页
 * 文件 : kernel/arch/paging.h
 * 说明 : 4KB 粒度的 32 位二级分页；页目录/页表由 PMM 动态分配
 * ============================================================ */
#ifndef YUAOS_ARCH_PAGING_H
#define YUAOS_ARCH_PAGING_H

#include "types.h"

/* ---- 页目录 / 页表条目标志 ---- */
#define PTE_PRESENT   0x001u
#define PTE_RW        0x002u
#define PTE_USER      0x004u
#define PTE_PWT       0x008u
#define PTE_PCD       0x010u
#define PTE_ACCESSED  0x020u
#define PTE_DIRTY     0x040u
#define PTE_PS        0x080u     /* 大页（仅 PDE 使用；本号不启用） */
#define PTE_GLOBAL    0x100u

#define PTE_ADDR_MASK 0xFFFFF000u

/* ---- 生命周期 ---- */
void vmm_init(void);

/* ---- 4KB 映射操作 ----
 * 返回：0 = 成功；-1 = 失败（PMM 无页 / 目标是大页 / 页表缺失）
 */
int  vmm_map_page  (u32 virt, u32 phys, u32 flags);
int  vmm_unmap_page(u32 virt);

/* 返回 virt 对应的物理地址；未映射返回 0 */
u32  vmm_get_phys  (u32 virt);

/* 用于调试：返回页目录的虚拟地址（= 物理地址） */
u32* vmm_get_page_directory(void);

#endif /* YUAOS_ARCH_PAGING_H */
