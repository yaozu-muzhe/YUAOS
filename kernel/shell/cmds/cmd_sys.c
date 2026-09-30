/* ============================================================
 * YUAOS - 系统命令组实现
 * 文件 : kernel/shell/cmds/cmd_sys.c
 * ============================================================ */
#include "shell/cmds/cmd_sys.h"
#include "shell/shell.h"
#include "fs/pcfs.h"
#include "loader/qa.h"
#include "task/task.h"
#include "arch/rtc.h"
#include "arch/cpu.h"
#include "mm/pmm.h"
#include "mm/heap.h"
#include "arch/paging.h"
#include "time.h"
#include "lib/io.h"
#include "lib/string.h"
#include "lib/printf.h"

/* ============================================================
 * del <name> in <path>      /      del whole
 * ============================================================ */
int cmd_del(int argc, char** argv)
{
    if (argc == 2 && strcmp(argv[1], "whole") == 0) {
        fs_reset();
        fs_node_t* home = fs_lookup(fs_root(), "/home/yuaos");
        shell_set_cwd(home ? home : fs_root());
        kprintf("%s\n", i18n_str(STR_MSG_RESET_DONE));
        return 0;
    }
    if (argc != 4 || strcmp(argv[2], "in") != 0) {
        shell_error("BadSyntax", "del <name> in <path>  |  del whole");
        return 1;
    }

    fs_node_t* cwd = shell_get_cwd();
    fs_node_t* dir = fs_lookup(cwd, argv[3]);
    if (!dir)                       { shell_error("PathNotFound", argv[3]); return 1; }
    if (dir->type != FS_NODE_DIR)   { shell_error("NotADir",      argv[3]); return 1; }

    fs_node_t* tgt = NULL;
    for (fs_node_t* c = dir->first_child; c; c = c->next_sibling) {
        if (strcmp(c->name, argv[1]) == 0) { tgt = c; break; }
    }
    if (!tgt) { shell_error("PathNotFound", argv[1]); return 1; }

    if (tgt->type == FS_NODE_DIR && tgt->first_child) {
        shell_error("DirNotEmpty", argv[1]);
        return 1;
    }

    fs_remove(tgt);
    kprintf("%s\n", i18n_str(STR_MSG_DELETED));
    return 0;
}

/* ============================================================
 * change lan <lang>
 * ============================================================ */
int cmd_change_lan(int argc, char** argv)
{
    if (argc != 3 || strcmp(argv[1], "lan") != 0) {
        shell_error("BadSyntax", "change lan <zh|jp|my|de|en|fr>");
        return 1;
    }
    lang_id_t id = i18n_parse(argv[2]);
    if (id >= LANG_COUNT) {
        shell_error("LanguageUnknown", argv[2]);
        kprintf("%s\n", i18n_str(STR_MSG_LANG_LIST));
        return 1;
    }
    i18n_set(id);
    kprintf(i18n_str(STR_MSG_LANG_CHANGED), i18n_name(id));
    kprintf("\n");
    return 0;
}

/* ============================================================
 * view <file> [line <N>]
 * ============================================================ */
static void view_print_line(const char* data, u32 size, int target)
{
    int n = 1;
    u32 start = 0;
    for (u32 i = 0; i <= size; i++) {
        if (i == size || data[i] == '\n') {
            if (n == target) {
                for (u32 j = start; j < i; j++) kprintf("%c", data[j]);
                kprintf("\n");
                return;
            }
            n++;
            start = i + 1;
        }
    }
    kprintf("%s\n", i18n_str(STR_MSG_LINE_OOR));
}

int cmd_view(int argc, char** argv)
{
    if (argc != 2 && argc != 4) {
        shell_error("BadSyntax", "view <file> [line <N>]");
        return 1;
    }
    int target_line = 0;
    if (argc == 4) {
        if (strcmp(argv[2], "line") != 0) {
            shell_error("BadSyntax", "view <file> line <N>");
            return 1;
        }
        const char* n = argv[3];
        for (u32 i = 0; n[i]; i++) {
            if (n[i] < '0' || n[i] > '9') {
                shell_error("BadNumber", n);
                return 1;
            }
            target_line = target_line * 10 + (n[i] - '0');
        }
        if (target_line <= 0) {
            shell_error("BadNumber", n);
            return 1;
        }
    }

    fs_node_t* f = fs_lookup(shell_get_cwd(), argv[1]);
    if (!f)                       { shell_error("PathNotFound", argv[1]); return 1; }
    if (f->type != FS_NODE_FILE)  { shell_error("NotAFile",    argv[1]); return 1; }

    if (f->size == 0) {
        kprintf("%s\n", i18n_str(STR_MSG_FILE_EMPTY));
        return 0;
    }

    if (target_line > 0) {
        view_print_line(f->data, f->size, target_line);
    } else {
        for (u32 i = 0; i < f->size; i++) kprintf("%c", f->data[i]);
        if (f->size > 0 && f->data[f->size - 1] != '\n') kprintf("\n");
    }
    return 0;
}

/* ============================================================
 * open <file>   —— .qa 走加载器 / 任务；其他走逐行脚本
 * ============================================================ */
int cmd_open(int argc, char** argv)
{
    if (argc != 2) {
        shell_error("BadSyntax", "open <file>");
        return 1;
    }
    fs_node_t* f = fs_lookup(shell_get_cwd(), argv[1]);
    if (!f)                       { shell_error("PathNotFound", argv[1]); return 1; }
    if (f->type != FS_NODE_FILE)  { shell_error("NotAFile",    argv[1]); return 1; }

    if (f->size == 0) {
        kprintf("%s\n", i18n_str(STR_MSG_FILE_EMPTY));
        return 0;
    }

    /* ---- .qa 可执行文件 ---- */
    if (qa_is_valid_file(f)) {
        kprintf(i18n_str(STR_MSG_QA_RUNNING), argv[1]);
        kprintf("\n");

        int exit_code = 0;
        int r = qa_load_and_run(argv[1], f, &exit_code);

        if (r == 0) {
            kprintf(i18n_str(STR_MSG_QA_DONE), exit_code);
            kprintf("\n");
        } else if (r > 0) {
            kprintf(i18n_str(STR_MSG_QA_ABORTED), r);
            kprintf("\n");
        } else {
            shell_error("QAError", argv[1]);
        }
        return (r == 0) ? 0 : 1;
    }

    /* ---- 其他：逐行 shell 脚本 ---- */
    kprintf(i18n_str(STR_MSG_OPEN_RUNNING), argv[1]);
    kprintf("\n");

    char line[SHELL_LINE_MAX];
    u32  llen = 0;
    for (u32 i = 0; i <= f->size; i++) {
        char c = (i < f->size) ? f->data[i] : '\n';
        if (c == '\n' || c == '\r') {
            if (llen == 0 && c == '\r') continue;
            line[llen] = 0;
            if (llen > 0) shell_execute_line(line);
            llen = 0;
        } else {
            if (llen + 1 < SHELL_LINE_MAX) line[llen++] = c;
        }
    }
    return 0;
}

/* ============================================================
 * bin <file>
 * ============================================================ */
int cmd_bin(int argc, char** argv)
{
    if (argc != 2) {
        shell_error("BadSyntax", "bin <file>");
        return 1;
    }
    fs_node_t* f = fs_lookup(shell_get_cwd(), argv[1]);
    if (!f)                       { shell_error("PathNotFound", argv[1]); return 1; }
    if (f->type != FS_NODE_FILE)  { shell_error("NotAFile",    argv[1]); return 1; }
    if (f->size == 0) {
        kprintf("%s\n", i18n_str(STR_MSG_FILE_EMPTY));
        return 0;
    }

    for (u32 i = 0; i < f->size; i++) {
        u8 b = (u8)f->data[i];
        for (int j = 7; j >= 0; j--) kprintf("%c", (b & (1u << j)) ? '1' : '0');
        kprintf(" ");
        if ((i & 7) == 7) kprintf("\n");
    }
    if (f->size % 8) kprintf("\n");
    return 0;
}

/* ============================================================
 * ba <file>
 * ============================================================ */
static const char* replace_ext(const char* src, const char* old_ext,
                               const char* new_ext, char* out, u32 cap)
{
    u32 sl = strlen(src);
    u32 ol = strlen(old_ext);
    if (sl <= ol) return NULL;
    if (strcmp(src + (sl - ol), old_ext) != 0) return NULL;
    u32 base = sl - ol;
    if (base + strlen(new_ext) + 1 > cap) return NULL;
    for (u32 i = 0; i < base; i++) out[i] = src[i];
    u32 k = base;
    for (u32 i = 0; new_ext[i]; i++) out[k++] = new_ext[i];
    out[k] = 0;
    return out;
}

int cmd_ba(int argc, char** argv)
{
    if (argc != 2) {
        shell_error("BadSyntax", "ba <file.exe|file.bat>");
        return 1;
    }
    const char* path = argv[1];

    char newpath[FS_PATH_MAX];
    const char* npx = NULL;
    if ((npx = replace_ext(path, ".exe", ".qa",  newpath, sizeof(newpath))) != NULL) {
    } else if ((npx = replace_ext(path, ".bat", ".set", newpath, sizeof(newpath))) != NULL) {
    } else {
        shell_error("BAUnsupported", path);
        return 1;
    }

    fs_node_t* src = fs_lookup(shell_get_cwd(), path);
    if (!src)                     { shell_error("PathNotFound", path); return 1; }
    if (src->type != FS_NODE_FILE){ shell_error("NotAFile",    path); return 1; }

    const char* newname = strrchr(npx, '/');
    newname = newname ? newname + 1 : npx;

    if (fs_rename(src, newname) != 0) {
        shell_error("RenameFailed", newname);
        return 1;
    }
    kprintf(i18n_str(STR_MSG_BA_DONE), path, npx);
    kprintf("\n");
    return 0;
}

/* ============================================================
 * time
 * ============================================================ */
int cmd_time(int argc, char** argv)
{
    (void)argc; (void)argv;
    rtc_time_t t;
    rtc_read(&t);
    kprintf("%04u-%02u-%02u %02u:%02u:%02u\n",
            (u32)t.year, (u32)t.month, (u32)t.day,
            (u32)t.hour, (u32)t.minute, (u32)t.second);
    return 0;
}

/* ============================================================
 * check
 * ============================================================ */
int cmd_check(int argc, char** argv)
{
    (void)argc; (void)argv;
    kprintf("%s\n", i18n_str(STR_MSG_CHECK_HEADER));
    kprintf("  CPU       : i686 (32-bit protected mode)\n");
    kprintf("  GDT       : flat, 5 gates\n");
    kprintf("  IDT       : 256 vectors\n");
    kprintf("  PIC       : remapped IRQ0-15 -> INT 32-47\n");
    kprintf("  PIT       : 100 Hz\n");
    kprintf("  PMM       : %u pages total, %u free\n",
            pmm_total_pages(), pmm_free_pages());
    kprintf("  Heap      : %u B total, %u B used, %u blocks\n",
            heap_total(), heap_used(), heap_block_count());
    kprintf("  VMM       : paging ON, identity-map 0 - 1GB\n");
    kprintf("  PCFS      : %u files, %u dirs\n",
            fs_count_files(), fs_count_dirs());
    kprintf("  Task      : %s\n",
            task_is_running() ? "running" : "idle");
    kprintf("  Ticks     : %u (uptime %u.%02u s)\n",
            (u32)kernel_ticks(), kernel_seconds(), kernel_centis());
    return 0;
}

/* ============================================================
 * % <path>
 * ============================================================ */
int cmd_path(int argc, char** argv)
{
    if (argc != 2) {
        const char* env = shell_get_path_env();
        kprintf("Path = %s\n", (env && *env) ? env : "(empty)");
        return 0;
    }
    if (shell_path_add(argv[1]) != 0) {
        shell_error("PathEnvFull", argv[1]);
        return 1;
    }
    kprintf(i18n_str(STR_MSG_PATH_ADDED), argv[1]);
    kprintf("\n");
    return 0;
}

/* ============================================================
 * System.sleep()
 * ============================================================ */
int cmd_sleep(int argc, char** argv)
{
    (void)argc; (void)argv;
    kprintf("%s\n", i18n_str(STR_MSG_SLEEPING));
    u64 target = kernel_ticks() + 100;
    while (kernel_ticks() < target) cpu_hlt();
    kprintf("%s\n", i18n_str(STR_MSG_WAKEUP));
    return 0;
}

/* ============================================================
 * System.close()
 * ============================================================ */
int cmd_close(int argc, char** argv)
{
    (void)argc; (void)argv;
    kprintf("%s\n", i18n_str(STR_MSG_SHUTDOWN));
    outw(0x604,  0x2000);
    outw(0xB004, 0x2000);
    kprintf("%s\n", i18n_str(STR_MSG_SHUTDOWN_FAILED));
    cpu_halt_forever();
    return 0;
}
