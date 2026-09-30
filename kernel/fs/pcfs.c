/* ============================================================
 * YUAOS - PCFS 内存实现
 * 文件 : kernel/fs/pcfs.c
 * ============================================================ */
#include "fs/pcfs.h"
#include "mm/heap.h"
#include "lib/string.h"

static fs_node_t* g_root = NULL;

/* ---------- 分配 ---------- */
static fs_node_t* node_alloc(const char* name, u8 type)
{
    fs_node_t* n = (fs_node_t*)kzalloc(sizeof(fs_node_t));
    if (!n) return NULL;
    strncpy(n->name, name, FS_NAME_MAX - 1);
    n->name[FS_NAME_MAX - 1] = 0;
    n->type = type;
    return n;
}

static void node_free(fs_node_t* n)
{
    if (!n) return;
    if (n->data) kfree(n->data);
    kfree(n);
}

static void free_recursive(fs_node_t* n)
{
    fs_node_t* c = n->first_child;
    while (c) {
        fs_node_t* next = c->next_sibling;
        free_recursive(c);
        c = next;
    }
    node_free(n);
}

/* ---------- 默认树（由 pcfs_init.c 提供） ---------- */
extern void pcfs_build_default_tree(void);

void fs_init(void)
{
    if (g_root) return;
    g_root = node_alloc("/", FS_NODE_DIR);
    if (!g_root) return;
    pcfs_build_default_tree();
}

void fs_clear_user_tree(void)
{
    if (!g_root) return;
    fs_node_t* c = g_root->first_child;
    while (c) {
        fs_node_t* next = c->next_sibling;
        free_recursive(c);
        c = next;
    }
    g_root->first_child = NULL;
}

void fs_reset(void)
{
    if (!g_root) { fs_init(); return; }
    fs_clear_user_tree();
    pcfs_build_default_tree();
}

fs_node_t* fs_root(void) { return g_root; }

/* ---------- 节点操作 ---------- */

fs_node_t* fs_create_child(fs_node_t* dir, const char* name, fs_node_type_t type)
{
    if (!dir || dir->type != FS_NODE_DIR) return NULL;
    if (!name || !*name) return NULL;
    if (strlen(name) >= FS_NAME_MAX) return NULL;
    if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) return NULL;

    for (fs_node_t* c = dir->first_child; c; c = c->next_sibling) {
        if (strcmp(c->name, name) == 0) return NULL;
    }

    fs_node_t* n = node_alloc(name, (u8)type);
    if (!n) return NULL;
    n->parent = dir;
    n->next_sibling = dir->first_child;
    dir->first_child = n;
    return n;
}

int fs_remove(fs_node_t* node)
{
    if (!node || !node->parent) return -1;
    fs_node_t* p = node->parent;
    fs_node_t** pp = &p->first_child;
    while (*pp) {
        if (*pp == node) { *pp = node->next_sibling; break; }
        pp = &(*pp)->next_sibling;
    }
    free_recursive(node);
    return 0;
}

int fs_write_file(fs_node_t* file, const void* data, u32 size)
{
    if (!file || file->type != FS_NODE_FILE) return -1;
    if (file->data) { kfree(file->data); file->data = NULL; file->size = 0; }
    if (size == 0) return 0;
    file->data = (char*)kmalloc(size);
    if (!file->data) return -1;
    memcpy(file->data, data, size);
    file->size = size;
    return 0;
}

int fs_rename(fs_node_t* node, const char* new_name)
{
    if (!node || !new_name || !*new_name) return -1;
    if (strlen(new_name) >= FS_NAME_MAX) return -1;
    if (node->parent) {
        for (fs_node_t* c = node->parent->first_child; c; c = c->next_sibling) {
            if (c != node && strcmp(c->name, new_name) == 0) return -1;
        }
    }
    strncpy(node->name, new_name, FS_NAME_MAX - 1);
    node->name[FS_NAME_MAX - 1] = 0;
    return 0;
}

/* ---------- 路径 ---------- */

static const char* skip_slash(const char* s) { while (*s == '/') s++; return s; }

static const char* next_segment(const char* s, char* out, u32 cap)
{
    s = skip_slash(s);
    u32 i = 0;
    while (*s && *s != '/') {
        if (i + 1 < cap) out[i++] = *s;
        s++;
    }
    out[i] = 0;
    return s;
}

fs_node_t* fs_lookup(fs_node_t* base, const char* path)
{
    if (!g_root) return NULL;
    if (!path)   return NULL;
    if (*path == 0) return base ? base : g_root;

    fs_node_t* cur = (*path == '/') ? g_root : (base ? base : g_root);

    char seg[FS_NAME_MAX];
    const char* p = path;

    for (;;) {
        p = next_segment(p, seg, sizeof(seg));
        if (seg[0] == 0) return cur;

        if (strcmp(seg, ".") == 0) {
            /* no-op */
        } else if (strcmp(seg, "..") == 0) {
            if (cur->parent) cur = cur->parent;
        } else {
            if (cur->type != FS_NODE_DIR) return NULL;
            fs_node_t* hit = NULL;
            for (fs_node_t* c = cur->first_child; c; c = c->next_sibling) {
                if (strcmp(c->name, seg) == 0) { hit = c; break; }
            }
            if (!hit) return NULL;
            cur = hit;
        }
        if (*p == 0) return cur;
    }
}

void fs_abspath(fs_node_t* node, char* out, u32 cap)
{
    if (cap == 0) return;
    out[0] = 0;
    if (!node) return;
    if (node == g_root) { if (cap > 1) { out[0] = '/'; out[1] = 0; } return; }

    fs_node_t* chain[64];
    u32 n = 0;
    for (fs_node_t* c = node; c && c != g_root && n < 64; c = c->parent) {
        chain[n++] = c;
    }
    u32 pos = 0;
    if (pos < cap - 1) out[pos++] = '/';
    for (i32 i = (i32)n - 1; i >= 0; i--) {
        const char* nm = chain[i]->name;
        u32 nl = strlen(nm);
        for (u32 j = 0; j < nl && pos < cap - 1; j++) out[pos++] = nm[j];
        if (i > 0 && pos < cap - 1) out[pos++] = '/';
    }
    out[pos] = 0;
}

fs_node_t* fs_clone(fs_node_t* src, fs_node_t* new_parent, const char* new_name)
{
    if (!src || !new_parent) return NULL;
    if (new_parent->type != FS_NODE_DIR) return NULL;

    fs_node_t* dst = fs_create_child(new_parent, new_name, (fs_node_type_t)src->type);
    if (!dst) return NULL;

    if (src->type == FS_NODE_FILE) {
        if (src->size > 0 && src->data) {
            fs_write_file(dst, src->data, src->size);
        }
    } else {
        for (fs_node_t* c = src->first_child; c; c = c->next_sibling) {
            fs_clone(c, dst, c->name);
        }
    }
    return dst;
}

/* ---------- 统计 ---------- */

static void count_rec(fs_node_t* n, u32* files, u32* dirs)
{
    for (fs_node_t* c = n->first_child; c; c = c->next_sibling) {
        if (c->type == FS_NODE_FILE) (*files)++;
        else                          (*dirs)++;
        if (c->type == FS_NODE_DIR) count_rec(c, files, dirs);
    }
}

u32 fs_count_nodes(void)
{
    if (!g_root) return 0;
    u32 f = 0, d = 0;
    count_rec(g_root, &f, &d);
    return f + d + 1;
}

u32 fs_count_files(void)
{
    if (!g_root) return 0;
    u32 f = 0, d = 0;
    count_rec(g_root, &f, &d);
    return f;
}

u32 fs_count_dirs(void)
{
    if (!g_root) return 0;
    u32 f = 0, d = 0;
    count_rec(g_root, &f, &d);
    return d + 1;
}
