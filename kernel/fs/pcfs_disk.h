/* ============================================================
 * YUAOS - PCFS 磁盘持久化
 * 文件 : kernel/fs/pcfs_disk.h
 * 说明 : 序列化内存树 <-> 磁盘；
 *        磁盘无有效 magic 时返回 -1（由调用方决定是否格式化）
 * ============================================================ */
#ifndef YUAOS_FS_PCFS_DISK_H
#define YUAOS_FS_PCFS_DISK_H

#include "types.h"

#define PCFS_DISK_LBA_BASE   1024u          /* superblock 位置 */
#define PCFS_DISK_DATA_LBA   1025u          /* 数据起始位置 */
#define PCFS_DISK_MAX_BYTES  (256u * 1024u) /* 数据区上限 256 KB */

int pcfs_disk_load(void);       /* 0 = 成功；-1 = 无有效镜像 */
int pcfs_disk_save(void);       /* 0 = 成功 */

#endif
