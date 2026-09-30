/* ============================================================
 * YUAOS - E820 内存图访问实现
 * 文件 : kernel/mm/e820.c
 * ============================================================ */
#include "mm/e820.h"
#include "lib/printf.h"

const e820_map_t* e820_get_map(void)
{
    return (const e820_map_t*)E820_MAGIC_ADDR;
}

u32 e820_max_address(void)
{
    const e820_map_t* map = e820_get_map();
    u32 max = 0;
    for (u32 i = 0; i < map->count && i < E820_MAX_ENTRIES; i++) {
        u32 base = map->entries[i].base_lo;
        u32 len  = map->entries[i].len_lo;
        u32 end  = base + len;
        if (end > max) max = end;
    }
    return max;
}

void e820_dump(void)
{
    const e820_map_t* map = e820_get_map();
    kprintf("E820 map: %u entries\n", map->count);
    for (u32 i = 0; i < map->count && i < E820_MAX_ENTRIES; i++) {
        const e820_entry_t* e = &map->entries[i];
        const char* tag = (e->type == 1) ? "usable" :
                          (e->type == 2) ? "reserved" :
                          (e->type == 3) ? "ACPI-reclaim" :
                          (e->type == 4) ? "ACPI-NVS" :
                          (e->type == 5) ? "bad" : "?";
        kprintf("  [%u] base=0x%08X len=0x%08X type=%u (%s)\n",
                i, e->base_lo, e->len_lo, e->type, tag);
    }
}
