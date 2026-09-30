/* ============================================================
 * YUAOS - host 侧 PCFS 镜像生成器
 * 文件 : tools/mkfs_pcfs.c
 * 编译 : gcc tools/mkfs_pcfs.c -o tools/mkfs_pcfs.exe
 * 用法 : mkfs_pcfs <src_dir> <out_img>
 *
 * 序列化格式与 kernel/fs/pcfs_disk.c 完全一致：
 *   TAG_DIR_BEGIN (0x01)  nlen u16  name  <children...>  TAG_DIR_END (0x00)
 *   TAG_FILE      (0x02)  nlen u16  name  size u32  data
 * ============================================================ */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <dirent.h>
#include <sys/stat.h>

#define IMG_SIZE  (16u * 1024u * 1024u)
#define SB_LBA    1024u
#define DT_LBA    1025u
#define SEC_SIZE  512u
#define MAX_DATA  (256u * 1024u)

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;

static u8  g_buf[MAX_DATA];
static u32 g_pos     = 0;
static int g_overflow = 0;

static void emit(const void* p, u32 n)
{
    if (g_overflow) return;
    if (g_pos + n > MAX_DATA) { g_overflow = 1; return; }
    memcpy(g_buf + g_pos, p, n);
    g_pos += n;
}

static void e_u8 (u8  v) { emit(&v, 1); }
static void e_u16(u16 v) { emit(&v, 2); }
static void e_u32(u32 v) { emit(&v, 4); }

static int cmp_str(const void* a, const void* b)
{
    return strcmp(*(const char**)a, *(const char**)b);
}

static char** list_dir(const char* path, int* count)
{
    DIR* d = opendir(path);
    if (!d) { *count = 0; return NULL; }

    int cap = 32, n = 0;
    char** names = (char**)malloc(sizeof(char*) * cap);
    struct dirent* e;

    while ((e = readdir(d)) != NULL) {
        if (strcmp(e->d_name, ".") == 0 || strcmp(e->d_name, "..") == 0) continue;
        if (n >= cap) {
            cap *= 2;
            names = (char**)realloc(names, sizeof(char*) * cap);
        }
        names[n++] = strdup(e->d_name);
    }
    closedir(d);

    if (n > 0) qsort(names, n, sizeof(char*), cmp_str);
    *count = n;
    return names;
}

static void free_list(char** names, int n)
{
    if (!names) return;
    for (int i = 0; i < n; i++) free(names[i]);
    free(names);
}

/* 递归序列化一个目录 */
static void ser_dir(const char* dirpath, const char* name)
{
    u32 nlen = (u32)strlen(name);
    if (nlen == 0 || nlen >= 32) { g_overflow = 1; return; }

    e_u8(0x01);                /* TAG_DIR_BEGIN */
    e_u16((u16)nlen);
    emit(name, nlen);

    int n;
    char** names = list_dir(dirpath, &n);

    for (int i = 0; i < n && !g_overflow; i++) {
        char child[1024];
        snprintf(child, sizeof(child), "%s/%s", dirpath, names[i]);

        struct stat st;
        if (stat(child, &st) != 0) continue;

        if (S_ISDIR(st.st_mode)) {
            ser_dir(child, names[i]);
        } else if (S_ISREG(st.st_mode)) {
            FILE* f = fopen(child, "rb");
            if (!f) continue;

            fseek(f, 0, SEEK_END);
            long fsz = ftell(f);
            fseek(f, 0, SEEK_SET);
            if (fsz < 0) { fclose(f); continue; }

            u8* data = (u8*)malloc(fsz ? (size_t)fsz : 1);
            if (fsz > 0 && fread(data, 1, (size_t)fsz, f) != (size_t)fsz) {
                free(data); fclose(f); continue;
            }
            fclose(f);

            u32 fnlen = (u32)strlen(names[i]);
            if (fnlen == 0 || fnlen >= 32) { free(data); g_overflow = 1; break; }

            e_u8(0x02);                /* TAG_FILE */
            e_u16((u16)fnlen);
            emit(names[i], fnlen);     /* 先 name */
            e_u32((u32)fsz);           /* 后 size */
            if (fsz > 0) emit(data, (u32)fsz);

            free(data);
        }
    }

    free_list(names, n);
    e_u8(0x00);                    /* TAG_DIR_END */
}

static int serialize_root(const char* src)
{
    g_pos = 0; g_overflow = 0;

    int n;
    char** names = list_dir(src, &n);

    for (int i = 0; i < n && !g_overflow; i++) {
        char child[1024];
        snprintf(child, sizeof(child), "%s/%s", src, names[i]);

        struct stat st;
        if (stat(child, &st) != 0) continue;

        if (S_ISDIR(st.st_mode)) {
            ser_dir(child, names[i]);
        } else if (S_ISREG(st.st_mode)) {
            FILE* f = fopen(child, "rb");
            if (!f) continue;

            fseek(f, 0, SEEK_END);
            long fsz = ftell(f);
            fseek(f, 0, SEEK_SET);

            u8* data = (u8*)malloc(fsz ? (size_t)fsz : 1);
            if (fsz > 0) fread(data, 1, (size_t)fsz, f);
            fclose(f);

            u32 fnlen = (u32)strlen(names[i]);
            e_u8(0x02);
            e_u16((u16)fnlen);
            emit(names[i], fnlen);
            e_u32((u32)fsz);
            if (fsz > 0) emit(data, (u32)fsz);

            free(data);
        }
    }
    free_list(names, n);

    return g_overflow ? -1 : 0;
}

int main(int argc, char** argv)
{
    if (argc != 3) {
        fprintf(stderr, "usage: %s <src_dir> <out_img>\n", argv[0]);
        return 1;
    }
    const char* src = argv[1];
    const char* out = argv[2];

    struct stat st;
    if (stat(src, &st) != 0 || !S_ISDIR(st.st_mode)) {
        fprintf(stderr, "error: %s is not a directory\n", src);
        return 1;
    }

    if (serialize_root(src) != 0) {
        fprintf(stderr, "error: serialized data exceeds %u bytes\n", MAX_DATA);
        return 1;
    }

    printf("  mkfs_pcfs: serialized %u bytes\n", g_pos);

    u8* img = (u8*)calloc(1, IMG_SIZE);
    if (!img) { perror("calloc"); return 1; }

    /* superblock */
    u8* sb = img + SB_LBA * SEC_SIZE;
    sb[0] = 'P'; sb[1] = 'C'; sb[2] = 'F'; sb[3] = 'S';
    *(u32*)(sb + 4) = 1;
    *(u32*)(sb + 8) = g_pos;

    /* data */
    memcpy(img + DT_LBA * SEC_SIZE, g_buf, g_pos);

    /* write */
    FILE* fp = fopen(out, "wb");
    if (!fp) { perror("fopen"); free(img); return 1; }
    if (fwrite(img, 1, IMG_SIZE, fp) != IMG_SIZE) {
        perror("fwrite"); fclose(fp); free(img); return 1;
    }
    fclose(fp);
    free(img);

    printf("  mkfs_pcfs: wrote %s (%u MB)\n", out, IMG_SIZE / 1024 / 1024);
    return 0;
}
