/* ============================================================
 * YUAOS - PCFS 磁盘序列化实现
 * 文件 : kernel/fs/pcfs_disk.c
 *
 * 格式：
 *   Superblock (LBA 1024, 512B)
 *     [0..3]   magic "PCFS"
 *     [4..7]   version u32 = 1
 *     [8..11]  data_bytes u32
 *     [12..15] reserved
 *
 *   数据区 (LBA 1025+，前序序列化)
 *     TAG_DIR_BEGIN(0x01) name_len u16 name
 *     TAG_FILE     (0x02) name_len u16 size u32 name data
 *     TAG_DIR_END  (0x00)
 * ============================================================ */
#include "fs/pcfs_disk.h"
#include "fs/pcfs.h"
#include "drivers/ata.h"
#include "mm/heap.h"
#include "lib/string.h"

#define TAG_DIR_END   0x00
#define TAG_DIR_BEGIN 0x01
#define TAG_FILE      0x02

#define SB_LBA  PCFS_DISK_LBA_BASE
#define DT_LBA  PCFS_DISK_DATA_LBA

/* ============================================================
 * 序列化上下文
 * ============================================================ */
typedef struct {
    u8* buf;
    u32 cap;
    u32 pos;
    int overflow;
} ser_t;

static void ser_bytes(ser_t* s, const void* p, u32 n)
{
    if (s->overflow) return;
    if (s->pos + n > s->cap) { s->overflow = 1; return; }
    memcpy(s->buf + s->pos, p, n);
    s->pos += n;
}

static void ser_u8 (ser_t* s, u8  v) { ser_bytes(s, &v, 1); }
static void ser_u16(ser_t* s, u16 v) { ser_bytes(s, &v, 2); }
static void ser_u32(ser_t* s, u32 v) { ser_bytes(s, &v, 4); }

static void serialize_node(ser_t* s, fs_node_t* n)
{
    if (s->overflow) return;

    u32 nlen = strlen(n->name);

    if (n->type == FS_NODE_DIR) {
        ser_u8 (s, TAG_DIR_BEGIN);
        ser_u16(s, (u16)nlen);
        ser_bytes(s, n->name, nlen);
        for (fs_node_t* c = n->first_child; c; c = c->next_sibling) {
            serialize_node(s, c);
        }
        ser_u8(s, TAG_DIR_END);
        } else {
        ser_u8 (s, TAG_FILE);
        ser_u16(s, (u16)nlen);
        ser_bytes(s, n->name, nlen);   /* ← 先 name */
        ser_u32(s, n->size);            /* ← 后 size */
        if (n->size > 0 && n->data) ser_bytes(s, n->data, n->size);
    }
}

/* ============================================================
 * 反序列化上下文
 * ============================================================ */
typedef struct {
    const u8* buf;
    u32 cap;
    u32 pos;
} de_t;

static int de_bytes(de_t* d, void* out, u32 n)
{
    if (d->pos + n > d->cap) return -1;
    memcpy(out, d->buf + d->pos, n);
    d->pos += n;
    return 0;
}

static int de_u8 (de_t* d, u8*  v) { return de_bytes(d, v, 1); }
static int de_u16(de_t* d, u16* v) { return de_bytes(d, v, 2); }
static int de_u32(de_t* d, u32* v) { return de_bytes(d, v, 4); }

/* 递归反序列化一个节点到 parent 下 */
static int deserialize_node(de_t* d, fs_node_t* parent)
{
    if (d->pos >= d->cap) return -1;

    u8 tag;
    if (de_u8(d, &tag) != 0) return -1;

    if (tag == TAG_DIR_END) {
        /* 由调用方处理；这里正常情况不会遇到 */
        return -1;
    }

    u16 nlen;
    if (de_u16(d, &nlen) != 0) return -1;
    if (nlen == 0 || nlen >= FS_NAME_MAX) return -1;

    char name[FS_NAME_MAX];
    if (de_bytes(d, name, nlen) != 0) return -1;
    name[nlen] = 0;

    if (tag == TAG_FILE) {
        u32 size;
        if (de_u32(d, &size) != 0) return -1;

        fs_node_t* f = fs_create_child(parent, name, FS_NODE_FILE);
        if (!f) return -1;

        if (size > 0) {
            if (size > PCFS_DISK_MAX_BYTES) return -1;
            u8* data = (u8*)kmalloc(size);
            if (!data) return -1;
            if (de_bytes(d, data, size) != 0) { kfree(data); return -1; }
            fs_write_file(f, data, size);
            kfree(data);
        }
        return 0;
    }

    if (tag == TAG_DIR_BEGIN) {
        fs_node_t* dir = fs_create_child(parent, name, FS_NODE_DIR);
        if (!dir) return -1;

        for (;;) {
            if (d->pos >= d->cap) return -1;
            u8 peek = d->buf[d->pos];
            if (peek == TAG_DIR_END) {
                d->pos++;                 /* 消耗结束标记 */
                return 0;
            }
            if (deserialize_node(d, dir) != 0) return -1;
        }
    }

    return -1;
}

/* ============================================================
 * 公共 API
 * ============================================================ */

int pcfs_disk_load(void)
{
    u8* sb = (u8*)kmalloc(512);
    if (!sb) return -1;

    if (ata_read_sectors(SB_LBA, 1, sb) != 0) { kfree(sb); return -1; }

    if (sb[0] != 'P' || sb[1] != 'C' || sb[2] != 'F' || sb[3] != 'S') {
        kfree(sb);
        return -1;
    }
    u32 version    = *(u32*)(sb + 4);
    u32 data_bytes = *(u32*)(sb + 8);
    kfree(sb);

    if (version != 1)             return -1;
    if (data_bytes == 0)          return -1;
    if (data_bytes > PCFS_DISK_MAX_BYTES) return -1;

    u32 sectors = (data_bytes + 511) / 512;
    u32 bytes   = sectors * 512;

    u8* buf = (u8*)kmalloc(bytes);
    if (!buf) return -1;

    /* 分批读（u8 count 上限 128） */
    u32 remaining = sectors;
    u32 lba = DT_LBA;
    u32 off = 0;
    while (remaining > 0) {
        u8 batch = (remaining > 128) ? 128 : (u8)remaining;
        if (ata_read_sectors(lba, batch, buf + off) != 0) {
            kfree(buf); return -1;
        }
        lba       += batch;
        off       += batch * 512;
        remaining -= batch;
    }

    /* 清空现有用户树，然后反序列化 */
    fs_clear_user_tree();

    de_t d = { buf, data_bytes, 0 };
    while (d.pos < d.cap) {
        if (deserialize_node(&d, fs_root()) != 0) {
            /* 出错则清空重来 */
            fs_clear_user_tree();
            kfree(buf);
            return -1;
        }
    }

    kfree(buf);
    return 0;
}

int pcfs_disk_save(void)
{
    u8* buf = (u8*)kmalloc(PCFS_DISK_MAX_BYTES);
    if (!buf) return -1;

    ser_t s = { buf, PCFS_DISK_MAX_BYTES, 0, 0 };
    for (fs_node_t* c = fs_root()->first_child; c; c = c->next_sibling) {
        serialize_node(&s, c);
    }
    if (s.overflow) { kfree(buf); return -1; }

    u32 data_bytes = s.pos;

    /* ---- 写 superblock ---- */
    u8 sb[512];
    memset(sb, 0, 512);
    sb[0] = 'P'; sb[1] = 'C'; sb[2] = 'F'; sb[3] = 'S';
    *(u32*)(sb + 4) = 1;
    *(u32*)(sb + 8) = data_bytes;

    if (ata_write_sectors(SB_LBA, 1, sb) != 0) { kfree(buf); return -1; }

    /* ---- 写数据区 ---- */
    u32 sectors = (data_bytes + 511) / 512;
    if (sectors > 0) {
        /* 末扇区补零 */
        for (u32 i = data_bytes; i < sectors * 512; i++) buf[i] = 0;

        u32 remaining = sectors;
        u32 lba = DT_LBA;
        u32 off = 0;
        while (remaining > 0) {
            u8 batch = (remaining > 128) ? 128 : (u8)remaining;
            if (ata_write_sectors(lba, batch, buf + off) != 0) {
                kfree(buf); return -1;
            }
            lba       += batch;
            off       += batch * 512;
            remaining -= batch;
        }
    }

    ata_flush();
    kfree(buf);
    return 0;
}
