/* ============================================================
 * YUAOS - ATA PIO 驱动实现
 * 文件 : kernel/drivers/ata.c
 * ============================================================ */
#include "drivers/ata.h"
#include "lib/io.h"

/* 主 IDE 通道 */
#define ATA_DATA       0x1F0
#define ATA_ERROR      0x1F1
#define ATA_FEATURES   0x1F1
#define ATA_SECCOUNT   0x1F2
#define ATA_LBA_LO     0x1F3
#define ATA_LBA_MID    0x1F4
#define ATA_LBA_HI     0x1F5
#define ATA_DRV_HEAD   0x1F6
#define ATA_STATUS     0x1F7
#define ATA_COMMAND    0x1F7
#define ATA_ALT_STATUS 0x3F6

#define ATA_SR_BSY     0x80
#define ATA_SR_DRDY    0x40
#define ATA_SR_DF      0x20
#define ATA_SR_DRQ     0x08
#define ATA_SR_ERR     0x01

#define ATA_CMD_READ     0x20
#define ATA_CMD_WRITE    0x30
#define ATA_CMD_FLUSH    0xE7
#define ATA_CMD_IDENTIFY 0xEC

static void ata_delay_400ns(void)
{
    for (int i = 0; i < 4; i++) (void)inb(ATA_ALT_STATUS);
}

static int ata_wait_not_busy(void)
{
    for (u32 i = 0; i < 1000000; i++) {
        if (!(inb(ATA_STATUS) & ATA_SR_BSY)) return 0;
    }
    return -1;
}

static int ata_wait_drq(void)
{
    for (u32 i = 0; i < 1000000; i++) {
        u8 s = inb(ATA_STATUS);
        if (s & (ATA_SR_ERR | ATA_SR_DF)) return -1;
        if (!(s & ATA_SR_BSY) && (s & ATA_SR_DRQ)) return 0;
    }
    return -1;
}

int ata_init(void)
{
    /* 选择主盘 */
    outb(ATA_DRV_HEAD, 0xA0);
    ata_delay_400ns();

    outb(ATA_SECCOUNT, 0);
    outb(ATA_LBA_LO,   0);
    outb(ATA_LBA_MID,  0);
    outb(ATA_LBA_HI,   0);
    outb(ATA_COMMAND,  ATA_CMD_IDENTIFY);
    ata_delay_400ns();

    u8 st = inb(ATA_STATUS);
    if (st == 0) return -1;                 /* 无设备 */
    if (ata_wait_not_busy() != 0) return -1;

    st = inb(ATA_STATUS);
    if (st & ATA_SR_ERR) return -1;

    /* 等 DRQ */
    if (ata_wait_drq() != 0) return -1;

    /* 吃掉 256 字 IDENTIFY 数据 */
    for (int i = 0; i < 256; i++) (void)inw(ATA_DATA);

    return 0;
}

/* ---------------- 批量读写 ---------------- */

static int ata_io(u32 lba, u8 count, void* buf, u8 cmd)
{
    if (count == 0) return 0;
    if (count > 128) return -1;
    if (lba > 0x0FFFFFFFu) return -1;

    if (ata_wait_not_busy() != 0) return -1;

    outb(ATA_DRV_HEAD, (u8)(0xE0 | ((lba >> 24) & 0x0F)));
    outb(ATA_SECCOUNT, count);
    outb(ATA_LBA_LO,  (u8)( lba        & 0xFF));
    outb(ATA_LBA_MID, (u8)((lba >>  8) & 0xFF));
    outb(ATA_LBA_HI,  (u8)((lba >> 16) & 0xFF));
    outb(ATA_COMMAND, cmd);

    u16* p = (u16*)buf;
    for (u32 s = 0; s < count; s++) {
        if (ata_wait_drq() != 0) return -1;
        if (cmd == ATA_CMD_READ) {
            for (int i = 0; i < 256; i++) p[i] = inw(ATA_DATA);
        } else {
            for (int i = 0; i < 256; i++) outw(ATA_DATA, p[i]);
        }
        p += 256;
    }
    return 0;
}

int ata_read_sectors(u32 lba, u8 count, void* buf)
{
    return ata_io(lba, count, buf, ATA_CMD_READ);
}

int ata_write_sectors(u32 lba, u8 count, const void* buf)
{
    return ata_io(lba, count, (void*)buf, ATA_CMD_WRITE);
}

void ata_flush(void)
{
    if (ata_wait_not_busy() != 0) return;
    outb(ATA_COMMAND, ATA_CMD_FLUSH);
    ata_wait_not_busy();
}
