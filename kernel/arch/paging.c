/* ============================================================
 * YUAOS - 分页实现
 * 文件 : kernel/arch/paging.c
 * 说明 :
 *   - 32 位二级分页（PD -> PT -> 4KB 页）
 *   - 页目录、页表均从 PMM 动态申请
 *   - 恒等映射覆盖 [0, min(max_e820, 1GB))
 *   - 因为恒等映射成立，软件里「物理地址」可直接当指针用
 * ============================================================ */
#include "arch/paging.h"
#include "mm/pmm.h"
#include "mm/e820.h"
#include "lib/string.h"
#include "lib/printf.h"

#define PD_INDEX(v) (((v) >> 22) & 0x3FFu)
#define PT_INDEX(v) (((v) >> 12) & 0x3FFu)

static u32* page_directory = NULL;

/* ------------------------------------------------------------
 * 内联汇编
 * ------------------------------------------------------------ */
static inline void invlpg(u32 vaddr)
{
    __asm__ volatile ("invlpg (%0)" : : "r"(vaddr) : "memory");
}

static inline void load_cr3(u32 pd_pa)
{
    __asm__ volatile ("mov %0, %%cr3" : : "r"(pd_pa) : "memory");
}

static inline u32 read_cr0(void)
{
    u32 v;
    __asm__ volatile ("mov %%cr0, %0" : "=r"(v));
    return v;
}

static inline void write_cr0(u32 v)
{
    __asm__ volatile ("mov %0, %%cr0" : : "r"(v) : "memory");
}

/* ------------------------------------------------------------
 * ensure_table : 保证 virt 所在 PDE 指向一张有效页表
 * 返回 PTE 基址（虚拟 = 物理）；失败返回 NULL
 * ------------------------------------------------------------ */
static u32* ensure_table(u32 virt)
{
    u32 pdi = PD_INDEX(virt);

    if (!(page_directory[pdi] & PTE_PRESENT)) {
        u32 pt_pa = pmm_alloc_page();
        if (!pt_pa) return NULL;

        /* 物理页清零；此时若分页已开，则依赖于恒等映射 */
        memset((void*)pt_pa, 0, PAGE_SIZE);

        page_directory[pdi] = pt_pa | PTE_PRESENT | PTE_RW;
        /* 新表无旧 TLB 项，不需要 invlpg */
    }

    u32 pt_pa = page_directory[pdi] & PTE_ADDR_MASK;
    return (u32*)pt_pa;
}

/* ------------------------------------------------------------
 * 公共 API
 * ------------------------------------------------------------ */
int vmm_map_page(u32 virt, u32 phys, u32 flags)
{
    virt &= PTE_ADDR_MASK;
    phys &= PTE_ADDR_MASK;

    u32* pt = ensure_table(virt);
    if (!pt) return -1;

    pt[PT_INDEX(virt)] = phys | (flags & 0xFFFu) | PTE_PRESENT;
    invlpg(virt);
    return 0;
}

int vmm_unmap_page(u32 virt)
{
    virt &= PTE_ADDR_MASK;
    u32 pdi = PD_INDEX(virt);

    if (!(page_directory[pdi] & PTE_PRESENT)) return -1;
    if (page_directory[pdi] & PTE_PS)        return -1;   /* 大页，本号不支持 */

    u32  pt_pa = page_directory[pdi] & PTE_ADDR_MASK;
    u32* pt    = (u32*)pt_pa;

    pt[PT_INDEX(virt)] = 0;
    invlpg(virt);
    return 0;
}

u32 vmm_get_phys(u32 virt)
{
    u32 pdi = PD_INDEX(virt);
    if (!(page_directory[pdi] & PTE_PRESENT)) return 0;

    if (page_directory[pdi] & PTE_PS) {
        /* 4MB 大页（当前不产生，防御性保留） */
        return (page_directory[pdi] & 0xFFC00000u) + (virt & 0x3FFFFFu);
    }

    u32  pt_pa = page_directory[pdi] & PTE_ADDR_MASK;
    u32* pt    = (u32*)pt_pa;
    u32  pte   = pt[PT_INDEX(virt)];
    if (!(pte & PTE_PRESENT)) return 0;

    return (pte & PTE_ADDR_MASK) + (virt & 0xFFFu);
}

u32* vmm_get_page_directory(void)
{
    return page_directory;
}

/* ------------------------------------------------------------
 * vmm_init
 *   1. 从 PMM 拿页目录
 *   2. 计算恒等映射上界（≤ 1GB）
 *   3. 逐 4KB 建立 [0, to_map) 恒等映射
 *   4. 装载 CR3 并置 CR0.PG
 * ------------------------------------------------------------ */
void vmm_init(void)
{
    /* 1. 页目录 */
    u32 pd_pa = pmm_alloc_page();
    if (!pd_pa) {
        kprintf("VMM: cannot allocate page directory!\n");
        return;
    }
    page_directory = (u32*)pd_pa;
    memset(page_directory, 0, PAGE_SIZE);

    /* 2. 恒等映射上界 */
    u32 max = e820_max_address();
    if (max > 0x40000000u) max = 0x40000000u;   /* 硬上限 1GB */
    u32 to_map = PAGE_ALIGN_UP(max);
    if (to_map == 0) to_map = 0x00400000u;      /* 兜底 4MB */

    /* 3. 建立恒等映射 */
    for (u32 addr = 0; addr < to_map; addr += PAGE_SIZE) {
        u32* pt = ensure_table(addr);
        if (!pt) {
            kprintf("VMM: out of memory while mapping 0x%X\n", addr);
            return;
        }
        pt[PT_INDEX(addr)] = addr | PTE_PRESENT | PTE_RW;
    }

    /* 4. 启用分页 */
    load_cr3(pd_pa);
    write_cr0(read_cr0() | 0x80000000u);

    kprintf("[OK] Paging enabled: identity-map 0x0 - 0x%X (4KB pages)\n", to_map);
}
