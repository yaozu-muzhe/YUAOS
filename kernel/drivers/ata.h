/* ============================================================
 * YUAOS - ATA PIO 驱动
 * 文件 : kernel/drivers/ata.h
 * 说明 : 主 IDE 通道（0x1F0），28 位 LBA，PIO 模式
 * ============================================================ */
#ifndef YUAOS_DRIVERS_ATA_H
#define YUAOS_DRIVERS_ATA_H

#include "types.h"

#define ATA_SECTOR_SIZE 512

/* 0 = 主盘存在；-1 = 无设备或初始化失败 */
int ata_init(void);

/* count 不超过 128（PIO 单次命令限制 + 兼容） */
int ata_read_sectors (u32 lba, u8 count, void* buf);
int ata_write_sectors(u32 lba, u8 count, const void* buf);

/* 强制刷写设备写缓存 */
void ata_flush(void);

#endif
