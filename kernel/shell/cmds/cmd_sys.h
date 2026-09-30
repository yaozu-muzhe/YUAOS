/* ============================================================
 * YUAOS - 系统命令组
 * 文件 : kernel/shell/cmds/cmd_sys.h
 * ============================================================ */
#ifndef YUAOS_SHELL_CMD_SYS_H
#define YUAOS_SHELL_CMD_SYS_H

int cmd_del       (int argc, char** argv);
int cmd_change_lan(int argc, char** argv);
int cmd_view      (int argc, char** argv);
int cmd_open      (int argc, char** argv);
int cmd_bin       (int argc, char** argv);
int cmd_ba        (int argc, char** argv);
int cmd_time      (int argc, char** argv);
int cmd_check     (int argc, char** argv);
int cmd_path      (int argc, char** argv);
int cmd_sleep     (int argc, char** argv);
int cmd_close     (int argc, char** argv);

#endif
