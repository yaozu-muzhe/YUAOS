
/* ============================================================
 * YUAOS - 文件系统命令组实现（10 号 · 上）
 * 文件 : kernel/shell/cmds/cmd_fs.c
 * ============================================================ */
#include "shell/cmds/cmd_fs.h"
#include "shell/shell.h"
#include "fs/pcfs.h"
#include "lib/string.h"
#include "lib/printf.h"

/* ---------- go to <path> ---------- */
int cmd_go(int argc, char** argv)
{
    if (argc != 3 || strcmp(argv[1], "to") != 0) {
        shell_error("BadSyntax", "go to <path>");
        return 1;
    }
    fs_node_t* cwd = shell_get_cwd();
    fs_node_t* tgt = fs_lookup(cwd, argv[2]);
    if (!tgt)                       { shell_error("PathNotFound", argv[2]); return 1; }
    if (tgt->type != FS_NODE_DIR)   { shell_error("NotADir",      argv[2]); return 1; }

    shell_push_cwd();
    shell_set_cwd(tgt);
    return 0;
}

/* ---------- back to <path>   或   back to（回上一次） ---------- */
int cmd_back(int argc, char** argv)
{
    if (argc == 1) {
        if (!shell_pop_cwd()) {
            shell_error("NoHistory", NULL);
            return 1;
        }
        return 0;
    }
    if (argc != 3 || strcmp(argv[1], "to") != 0) {
        shell_error("BadSyntax", "back to [<path>]");
        return 1;
    }
    fs_node_t* cwd = shell_get_cwd();
    fs_node_t* tgt = fs_lookup(cwd, argv[2]);
    if (!tgt)                     { shell_error("PathNotFound", argv[2]); return 1; }
    if (tgt->type != FS_NODE_DIR) { shell_error("NotADir",      argv[2]); return 1; }

    shell_push_cwd();
    shell_set_cwd(tgt);
    return 0;
}

/* ---------- add <name> [in <path>] ---------- */
int cmd_add(int argc, char** argv)
{
    if (argc != 2 && argc != 4) {
        shell_error("BadSyntax", "add <name> [in <path>]");
        return 1;
    }
    const char* name = argv[1];
    fs_node_t* cwd = shell_get_cwd();
    fs_node_t* parent = cwd;

    if (argc == 4) {
        if (strcmp(argv[2], "in") != 0) {
            shell_error("BadSyntax", "add <name> in <path>");
            return 1;
        }
        parent = fs_lookup(cwd, argv[3]);
        if (!parent)                   { shell_error("PathNotFound", argv[3]); return 1; }
        if (parent->type != FS_NODE_DIR){ shell_error("NotADir",      argv[3]); return 1; }
    }

    /* 含 '.' 视为文件，否则目录 */
    fs_node_type_t type = (strchr(name, '.') != NULL) ? FS_NODE_FILE : FS_NODE_DIR;

    fs_node_t* n = fs_create_child(parent, name, type);
    if (!n) { shell_error("CreateFailed", name); return 1; }

    kprintf("%s\n", (type == FS_NODE_FILE)
                     ? i18n_str(STR_MSG_FILE_CREATED)
                     : i18n_str(STR_MSG_DIR_CREATED));
    return 0;
}

/* ---------- move <name> to <path> ---------- */
int cmd_move(int argc, char** argv)
{
    if (argc != 4 || strcmp(argv[2], "to") != 0) {
        shell_error("BadSyntax", "move <name> to <path>");
        return 1;
    }
    fs_node_t* cwd = shell_get_cwd();

    fs_node_t* src = fs_lookup(cwd, argv[1]);
    if (!src)                     { shell_error("PathNotFound", argv[1]); return 1; }
    if (src == fs_root())         { shell_error("RootProtected", NULL);   return 1; }

    fs_node_t* dst = fs_lookup(cwd, argv[3]);
    if (!dst)                     { shell_error("PathNotFound", argv[3]); return 1; }
    if (dst->type != FS_NODE_DIR) { shell_error("NotADir",      argv[3]); return 1; }

    for (fs_node_t* p = dst; p; p = p->parent) {
        if (p == src) { shell_error("MoveIntoSelf", argv[1]); return 1; }
    }

    /* 从原 parent 摘除 */
    fs_node_t* op = src->parent;
    fs_node_t** pp = &op->first_child;
    while (*pp) {
        if (*pp == src) { *pp = src->next_sibling; break; }
        pp = &(*pp)->next_sibling;
    }

    /* 挂到 dst */
    src->parent = dst;
    src->next_sibling = dst->first_child;
    dst->first_child = src;

    kprintf(i18n_str(STR_MSG_MOVED), argv[1], argv[3]);
    kprintf("\n");
    return 0;
}

/* ---------- list [<path>] ---------- */
int cmd_list(int argc, char** argv)
{
    fs_node_t* cwd = shell_get_cwd();
    fs_node_t* tgt = cwd;

    if (argc >= 2) {
        tgt = fs_lookup(cwd, argv[1]);
        if (!tgt) { shell_error("PathNotFound", argv[1]); return 1; }
    }

    if (tgt->type == FS_NODE_FILE) {
        char ap[FS_PATH_MAX];
        fs_abspath(tgt, ap, sizeof(ap));
        kprintf("  %s  %u bytes\n", ap, tgt->size);
        return 0;
    }

    if (!tgt->first_child) {
        kprintf("%s\n", i18n_str(STR_MSG_EMPTY_DIR));
        return 0;
    }

    for (fs_node_t* c = tgt->first_child; c; c = c->next_sibling) {
        if (c->type == FS_NODE_DIR) {
            kprintf("  d  %-24s  <DIR>\n", c->name);
        } else {
            kprintf("  -  %-24s  %u\n", c->name, c->size);
        }
    }
    return 0;
}

/* ---------- copy <name> in <src> and add to <dst> ---------- */
int cmd_copy(int argc, char** argv)
{
    /* 期望 8 个 token：
     *   copy <name> in <src> and add to <dst>
     *   [0]  [1]   [2] [3]   [4] [5] [6] [7] */
    if (argc != 8 ||
        strcmp(argv[2], "in")  != 0 ||
        strcmp(argv[4], "and") != 0 ||
        strcmp(argv[5], "add") != 0 ||
        strcmp(argv[6], "to")  != 0) {
        shell_error("BadSyntax",
                    "copy <name> in <src> and add to <dst>");
        return 1;
    }

    const char* name     = argv[1];
    const char* src_path = argv[3];
    const char* dst_path = argv[7];
    fs_node_t*  cwd      = shell_get_cwd();

    /* 拼出 <src>/<name> */
    char full[FS_PATH_MAX];
    u32 sl = strlen(src_path);
    if (sl + strlen(name) + 2 >= FS_PATH_MAX) {
        shell_error("PathTooLong", NULL);
        return 1;
    }
    strcpy(full, src_path);
    if (sl > 0 && src_path[sl - 1] != '/') {
        full[sl]     = '/';
        full[sl + 1] = 0;
    }
    strcat(full, name);

    fs_node_t* src = fs_lookup(cwd, full);
    if (!src) { shell_error("PathNotFound", full); return 1; }

    fs_node_t* dst = fs_lookup(cwd, dst_path);
    if (!dst)                     { shell_error("PathNotFound", dst_path); return 1; }
    if (dst->type != FS_NODE_DIR) { shell_error("NotADir",      dst_path); return 1; }

    if (src->type == FS_NODE_DIR) {
        for (fs_node_t* p = dst; p; p = p->parent) {
            if (p == src) { shell_error("CopyIntoSelf", name); return 1; }
        }
    }

    fs_node_t* cp = fs_clone(src, dst, name);
    if (!cp) { shell_error("CopyFailed", name); return 1; }

    kprintf(i18n_str(STR_MSG_COPIED), name, dst_path);
    kprintf("\n");
    return 0;
}
