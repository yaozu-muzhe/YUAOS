/* ============================================================
 * YUAOS - E820 内存图访问
 * 文件 : kernel/mm/e820.h
 * ============================================================ */
#ifndef YUAOS_MM_E820_H
#define YUAOS_MM_E820_H

#include "types.h"

#define E820_MAX_ENTRIES 128
#define E820_MAGIC_ADDR  0x9000u

typedef struct {
    u32 base_lo;
    u32 base_hi;
    u32 len_lo;
    u32 len_hi;
    u32 type;
    u32 acpi;
} __attribute__((packed)) e820_entry_t;

typedef struct {
    u32          count;
    e820_entry_t entries[E820_MAX_ENTRIES];
} __attribute__((packed)) e820_map_t;

#define E820_TYPE_USABLE 1

const e820_map_t* e820_get_map(void);
u32               e820_max_address(void);
void              e820_dump(void);

#endif /* YUAOS_MM_E820_H */
