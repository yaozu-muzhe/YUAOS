/* ============================================================
 * YUAOS - Shell
 * 文件 : kernel/shell/shell.h
 * ============================================================ */
#ifndef YUAOS_SHELL_SHELL_H
#define YUAOS_SHELL_SHELL_H

#include "types.h"
#include "fs/pcfs.h"
#include "shell/i18n/i18n.h"

#define SHELL_LINE_MAX  128
#define SHELL_ARG_MAX   16
#define SHELL_CWD_STACK 16
#define SHELL_PATH_MAX  256

void shell_init(void);
void shell_run(void);

/* ---- cwd ---- */
fs_node_t* shell_get_cwd(void);
void       shell_set_cwd(fs_node_t* dir);
void       shell_push_cwd(void);
int        shell_pop_cwd(void);

/* ---- 供 open 命令逐行执行脚本 ---- */
void shell_execute_line(const char* line);

/* ---- Path 环境变量 ---- */
const char* shell_get_path_env(void);
int         shell_path_add(const char* path);   /* 0=OK, -1=满 */

/* ---- 输出 ---- */
void shell_error(const char* type, const char* detail);
void shell_ok(str_id_t msg_id, const char* arg);

#endif
