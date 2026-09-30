/* ============================================================
 * YUAOS - Shell 实现
 * 文件 : kernel/shell/shell.c
 * ============================================================ */
#include "shell/shell.h"
#include "shell/cmds/cmd_fs.h"
#include "shell/cmds/cmd_sys.h"
#include "lib/printf.h"
#include "lib/string.h"
#include "drivers/vga.h"
#include "drivers/keyboard.h"
#include "arch/irq.h"
#include "fs/pcfs_disk.h"

/* ============================================================ cwd + Path */
static fs_node_t* g_cwd = NULL;
static fs_node_t* g_cwd_stack[SHELL_CWD_STACK];
static int        g_cwd_sp = 0;

/* Path 环境变量：以 ':' 分隔的绝对路径列表 */
static char g_path_env[SHELL_PATH_MAX] = "/bin";

fs_node_t* shell_get_cwd(void) { return g_cwd; }

void shell_set_cwd(fs_node_t* d) { if (d) g_cwd = d; }

void shell_push_cwd(void)
{
    if (g_cwd_sp < SHELL_CWD_STACK) g_cwd_stack[g_cwd_sp++] = g_cwd;
}

int shell_pop_cwd(void)
{
    if (g_cwd_sp == 0) return 0;
    g_cwd = g_cwd_stack[--g_cwd_sp];
    return 1;
}

const char* shell_get_path_env(void) { return g_path_env; }

int shell_path_add(const char* path)
{
    if (!path || !*path) return -1;
    u32 cur = strlen(g_path_env);
    u32 add = strlen(path);
    u32 need = cur + (cur ? 1 : 0) + add + 1;
    if (need > SHELL_PATH_MAX) return -1;
    if (cur) { g_path_env[cur++] = ':'; }
    for (u32 i = 0; i < add; i++) g_path_env[cur + i] = path[i];
    g_path_env[cur + add] = 0;
    return 0;
}

/* ============================================================ 历史 */
#define HIST_MAX 32
static char g_hist[HIST_MAX][SHELL_LINE_MAX];
static int  g_hist_count = 0;
static int  g_hist_browse = -1;

static void hist_add(const char* line)
{
    if (!line || !*line) return;
    if (g_hist_count > 0) {
        int last = (g_hist_count - 1) % HIST_MAX;
        if (strcmp(g_hist[last], line) == 0) return;
    }
    int idx = g_hist_count % HIST_MAX;
    strncpy(g_hist[idx], line, SHELL_LINE_MAX - 1);
    g_hist[idx][SHELL_LINE_MAX - 1] = 0;
    g_hist_count++;
    g_hist_browse = -1;
}

/* ============================================================ 分词 */
static int tokenize(char* line, int max, char** out)
{
    int n = 0;
    char* p = line;
    while (*p && n < max) {
        while (*p == ' ' || *p == '\t') p++;
        if (!*p) break;
        out[n++] = p;
        while (*p && *p != ' ' && *p != '\t') p++;
        if (*p) *p++ = 0;
    }
    return n;
}

/* ============================================================ 命令表 */
typedef struct {
    const char* name;
    str_id_t    desc;
    int       (*fn)(int argc, char** argv);
} shell_cmd_t;

static int cmd_help   (int argc, char** argv);
static int cmd_cls    (int argc, char** argv);
static int cmd_lang   (int argc, char** argv);
static int cmd_echo   (int argc, char** argv);
static int cmd_about  (int argc, char** argv);
static int cmd_history(int argc, char** argv);
static int cmd_pwd    (int argc, char** argv);

static const shell_cmd_t g_cmds[] = {
    { "help",           STR_CMD_HELP_DESC,      cmd_help        },
    { "cls",            STR_CMD_CLS_DESC,       cmd_cls         },
    { "lang",           STR_CMD_LANG_DESC,      cmd_lang        },
    { "echo",           STR_CMD_ECHO_DESC,      cmd_echo        },
    { "about",          STR_CMD_ABOUT_DESC,     cmd_about       },
    { "history",        STR_CMD_HISTORY_DESC,   cmd_history     },
    { "pwd",            STR_CMD_PWD_DESC,       cmd_pwd         },

    /* ---- 文件系统 ---- */
    { "go",             STR_CMD_GOTO_DESC,      cmd_go          },
    { "back",           STR_CMD_BACK_DESC,      cmd_back        },
    { "add",            STR_CMD_ADD_DESC,       cmd_add         },
    { "move",           STR_CMD_MOVE_DESC,      cmd_move        },
    { "list",           STR_CMD_LIST_DESC,      cmd_list        },
    { "copy",           STR_CMD_COPY_DESC,      cmd_copy        },

    /* ---- 系统（11 号）---- */
    { "del",            STR_CMD_DEL_DESC,       cmd_del         },
    { "change",         STR_CMD_CHANGELAN_DESC, cmd_change_lan  },
    { "view",           STR_CMD_VIEW_DESC,      cmd_view        },
    { "open",           STR_CMD_OPEN_DESC,      cmd_open        },
    { "bin",            STR_CMD_BIN_DESC,       cmd_bin         },
    { "ba",             STR_CMD_BA_DESC,        cmd_ba           },
    { "time",           STR_CMD_TIME_DESC,      cmd_time        },
    { "check",          STR_CMD_CHECK_DESC,     cmd_check       },
    { "%",              STR_CMD_PATH_DESC,      cmd_path        },
    { "System.sleep()", STR_CMD_SLEEP_DESC,     cmd_sleep       },
    { "System.close()", STR_CMD_CLOSE_DESC,     cmd_close       },
};
#define CMD_COUNT (sizeof(g_cmds) / sizeof(g_cmds[0]))

/* ============================================================ 输出 */
void shell_error(const char* type, const char* detail)
{
    kprintf("%s", type);
    if (detail) kprintf(": %s", detail);
    kprintf("  %s\n", i18n_str(STR_ERR_SUFFIX));
}

void shell_ok(str_id_t msg_id, const char* arg)
{
    const char* fmt = i18n_str(msg_id);
    if (arg) kprintf(fmt, arg);
    else     kprintf("%s", fmt);
    kprintf("\n");
}

/* ============================================================ 内建命令 */
static int cmd_help(int argc, char** argv)
{
    (void)argc; (void)argv;
    kprintf("%s\n", i18n_str(STR_HELP_HEADER));
    for (u32 i = 0; i < CMD_COUNT; i++) {
        kprintf("  %-16s %s\n", g_cmds[i].name, i18n_str(g_cmds[i].desc));
    }
    return 0;
}

static int cmd_cls(int argc, char** argv) { (void)argc; (void)argv; vga_clear(); return 0; }

static int cmd_lang(int argc, char** argv)
{
    if (argc < 2) {
        kprintf("%s\n", i18n_str(STR_MSG_LANG_LIST));
        return 0;
    }
    lang_id_t id = i18n_parse(argv[1]);
    if (id >= LANG_COUNT) {
        shell_error("LanguageUnknown", argv[1]);
        kprintf("%s\n", i18n_str(STR_MSG_LANG_LIST));
        return 1;
    }
    i18n_set(id);
    kprintf(i18n_str(STR_MSG_LANG_CHANGED), i18n_name(id));
    kprintf("\n");
    return 0;
}

static int cmd_echo(int argc, char** argv)
{
    for (int i = 1; i < argc; i++) {
        if (i > 1) kprintf(" ");
        kprintf("%s", argv[i]);
    }
    kprintf("\n");
    return 0;
}

static int cmd_about(int argc, char** argv)
{
    (void)argc; (void)argv;
    kprintf("%s\n", i18n_str(STR_ABOUT_LINE1));
    kprintf("%s\n", i18n_str(STR_ABOUT_LINE2));
    kprintf("%s\n", i18n_str(STR_ABOUT_LINE3));
    return 0;
}

static int cmd_history(int argc, char** argv)
{
    (void)argc; (void)argv;
    if (g_hist_count == 0) {
        kprintf("%s\n", i18n_str(STR_MSG_HISTORY_EMPTY));
        return 0;
    }
    int first = (g_hist_count > HIST_MAX) ? (g_hist_count - HIST_MAX) : 0;
    for (int i = first; i < g_hist_count; i++) {
        kprintf("  %3d  %s\n", i + 1, g_hist[i % HIST_MAX]);
    }
    return 0;
}

static int cmd_pwd(int argc, char** argv)
{
    (void)argc; (void)argv;
    char ap[FS_PATH_MAX];
    fs_abspath(g_cwd, ap, sizeof(ap));
    kprintf("%s\n", ap);
    return 0;
}

/* ============================================================ 行编辑 */
static void redraw_line(const char* line, u32 old_len)
{
    for (u32 i = 0; i < old_len; i++) kprintf("\b \b");
    kprintf("%s", line);
}

static int readline(char* buf, u32 cap, const char* prompt)
{
    u32 len = 0;
    buf[0] = 0;
    kprintf("%s", prompt);
    g_hist_browse = -1;

    for (;;) {
        int c = kb_wait_getchar();

        if (c == '\n') { kprintf("\n"); buf[len] = 0; return (int)len; }

        if (c == '\b') {
            if (len == 0) continue;
            kprintf("\b \b");
            len--;
            continue;
        }

        if (c == KB_KEY_UP) {
            if (g_hist_count == 0) continue;
            if (g_hist_browse < 0) g_hist_browse = g_hist_count - 1;
            else if (g_hist_browse > 0) g_hist_browse--;
            int idx = g_hist_browse % HIST_MAX;
            redraw_line(g_hist[idx], len);
            strncpy(buf, g_hist[idx], cap - 1);
            buf[cap - 1] = 0;
            len = strlen(buf);
            continue;
        }
        if (c == KB_KEY_DOWN) {
            if (g_hist_browse < 0) continue;
            g_hist_browse++;
            if (g_hist_browse >= g_hist_count) {
                g_hist_browse = -1;
                redraw_line("", len);
                len = 0;
                buf[0] = 0;
            } else {
                int idx = g_hist_browse % HIST_MAX;
                redraw_line(g_hist[idx], len);
                strncpy(buf, g_hist[idx], cap - 1);
                buf[cap - 1] = 0;
                len = strlen(buf);
            }
            continue;
        }
        if (c >= 0x20 && c < 0x80) {
            if (len + 1 >= cap) continue;
            buf[len++] = (char)c;
            kprintf("%c", (char)c);
        }
    }
}
static int needs_sync(const char* cmd)
{
    static const char* s_sync[] = {
        "add", "move", "copy", "del", "ba", "open", 0
    };
    for (int i = 0; s_sync[i]; i++) {
        if (strcmp(cmd, s_sync[i]) == 0) return 1;
    }
    return 0;
}

/* ============================================================ 执行 */
void shell_execute_line(const char* line)
{
    char work[SHELL_LINE_MAX];
    strncpy(work, line, SHELL_LINE_MAX - 1);
    work[SHELL_LINE_MAX - 1] = 0;

    char* argv[SHELL_ARG_MAX];
    int argc = tokenize(work, SHELL_ARG_MAX, argv);
    if (argc == 0) return;

    for (u32 i = 0; i < CMD_COUNT; i++) {
        if (strcmp(argv[0], g_cmds[i].name) == 0) {
            g_cmds[i].fn(argc, argv);
            if (needs_sync(argv[0])) pcfs_disk_save();
            return;
        }
    }
    shell_error("CommandNotFound", argv[0]);
}

/* ============================================================ 入口 */
void shell_init(void)
{
    i18n_init();
    kb_init();
    irq_register(1, kb_irq_handler);
    irq_unmask(1);

    g_cwd = fs_lookup(fs_root(), "/home/yuaos");
    if (!g_cwd) g_cwd = fs_root();
    g_cwd_sp = 0;
}

void shell_run(void)
{
    kprintf("\n");
    kprintf("%s\n", i18n_str(STR_WELCOME));
    kprintf("\n");

    char line[SHELL_LINE_MAX];
    for (;;) {
        int n = readline(line, sizeof(line), i18n_str(STR_PROMPT));
        if (n <= 0) continue;
        hist_add(line);
        shell_execute_line(line);
    }
}
