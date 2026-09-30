/* ============================================================
 * YUAOS - GDT 实现
 * 文件 : kernel/arch/gdt.c
 * 说明 : 平坦模型（base=0, limit=4G），4 个段 + 1 个空描述符
 * ============================================================ */
#include "arch/gdt.h"

typedef struct {
    u16 limit_lo;
    u16 base_lo;
    u8  base_mid;
    u8  access;
    u8  granularity;
    u8  base_hi;
} __attribute__((packed)) gdt_entry_t;

typedef struct {
    u16 limit;
    u32 base;
} __attribute__((packed)) gdt_ptr_t;

static gdt_entry_t gdt_entries[5];
static gdt_ptr_t   gdt_ptr;

extern void gdt_flush(u32 gdt_ptr_addr);

static void gdt_set_gate(int i, u32 base, u32 limit, u8 access, u8 flags)
{
    gdt_entries[i].base_lo     = (u16)(base & 0xFFFF);
    gdt_entries[i].base_mid    = (u8)((base >> 16) & 0xFF);
    gdt_entries[i].base_hi     = (u8)((base >> 24) & 0xFF);
    gdt_entries[i].limit_lo    = (u16)(limit & 0xFFFF);
    gdt_entries[i].granularity = (u8)(((limit >> 16) & 0x0F) | (flags & 0xF0));
    gdt_entries[i].access      = access;
}

void gdt_init(void)
{
    gdt_ptr.limit = (u16)(sizeof(gdt_entries) - 1);
    gdt_ptr.base  = (u32)&gdt_entries;

    gdt_set_gate(0, 0, 0,          0x00, 0x00); /* 空描述符 */
    gdt_set_gate(1, 0, 0x000FFFFF, 0x9A, 0xC0); /* 0x08 内核代码 */
    gdt_set_gate(2, 0, 0x000FFFFF, 0x92, 0xC0); /* 0x10 内核数据 */
    gdt_set_gate(3, 0, 0x000FFFFF, 0xFA, 0xC0); /* 0x18 用户代码 */
    gdt_set_gate(4, 0, 0x000FFFFF, 0xF2, 0xC0); /* 0x20 用户数据 */

    gdt_flush((u32)&gdt_ptr);
}
