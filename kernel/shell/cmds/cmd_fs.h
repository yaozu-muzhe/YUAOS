/* ============================================================
 * YUAOS - 文件系统命令组
 * 文件 : kernel/shell/cmds/cmd_fs.h
 * ============================================================ */
#ifndef YUAOS_SHELL_CMD_FS_H
#define YUAOS_SHELL_CMD_FS_H

int cmd_go   (int argc, char** argv);
int cmd_back (int argc, char** argv);
int cmd_add  (int argc, char** argv);
int cmd_move (int argc, char** argv);
int cmd_list (int argc, char** argv);
int cmd_copy (int argc, char** argv);

#endif
