/* ============================================================
 * YUAOS - PCFS 内存文件系统
 * 文件 : kernel/fs/pcfs.h
 * ============================================================ */
#ifndef YUAOS_FS_PCFS_H
#define YUAOS_FS_PCFS_H

#include "types.h"

#define FS_NAME_MAX 32
#define FS_PATH_MAX 256

typedef enum {
    FS_NODE_FILE = 0,
    FS_NODE_DIR  = 1
} fs_node_type_t;

typedef struct fs_node {
    char              name[FS_NAME_MAX];
    u8                type;
    struct fs_node*   parent;
    struct fs_node*   first_child;
    struct fs_node*   next_sibling;
    char*             data;
    u32               size;
} fs_node_t;

/* ---- 生命周期 ---- */
void        fs_init(void);
void        fs_reset(void);              /* 清空所有用户内容，重建默认树 */
fs_node_t*  fs_root(void);
void        fs_clear_user_tree(void);   /* 只清空子节点，保留 root */

/* ---- 节点操作 ---- */
fs_node_t*  fs_create_child(fs_node_t* dir, const char* name, fs_node_type_t type);
int         fs_remove(fs_node_t* node);
int         fs_write_file(fs_node_t* file, const void* data, u32 size);
int         fs_rename(fs_node_t* node, const char* new_name);

/* ---- 查找 ---- */
fs_node_t*  fs_lookup(fs_node_t* base, const char* path);
void        fs_abspath(fs_node_t* node, char* out, u32 cap);
fs_node_t*  fs_clone(fs_node_t* src, fs_node_t* new_parent, const char* new_name);

/* ---- 统计 ---- */
u32         fs_count_nodes(void);
u32         fs_count_files(void);
u32         fs_count_dirs(void);

#endif
