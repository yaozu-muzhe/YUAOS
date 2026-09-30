/* ============================================================
 * YUAOS - 国际化
 * 文件 : kernel/shell/i18n/i18n.h
 * ============================================================ */
#ifndef YUAOS_SHELL_I18N_H
#define YUAOS_SHELL_I18N_H

#include "types.h"

typedef enum {
    LANG_ZH = 0, LANG_JP, LANG_MY, LANG_DE, LANG_EN, LANG_FR, LANG_COUNT
} lang_id_t;

typedef enum {
    STR_WELCOME, STR_PROMPT, STR_ERR_SUFFIX,
    STR_MSG_OK, STR_MSG_LANG_CHANGED, STR_MSG_LANG_UNKNOWN,
    STR_MSG_LANG_LIST, STR_MSG_BYE, STR_MSG_SLEEPING,
    STR_MSG_HISTORY_EMPTY, STR_MSG_CMD_NOT_FOUND, STR_MSG_BAD_SYNTAX,
    STR_MSG_TOO_MANY_ARGS, STR_INPUT_TOO_LONG,

    STR_HELP_HEADER, STR_HELP_USAGE_PREFIX,

    STR_CMD_HELP_DESC, STR_CMD_CLS_DESC, STR_CMD_LANG_DESC,
    STR_CMD_ECHO_DESC, STR_CMD_ABOUT_DESC, STR_CMD_HISTORY_DESC,
    STR_CMD_PWD_DESC, STR_CMD_GOTO_DESC, STR_CMD_BACK_DESC,
    STR_CMD_ADD_DESC, STR_CMD_MOVE_DESC, STR_CMD_LIST_DESC, STR_CMD_COPY_DESC,

    STR_ABOUT_LINE1, STR_ABOUT_LINE2, STR_ABOUT_LINE3,

    STR_MSG_DIR_CREATED, STR_MSG_FILE_CREATED,
    STR_MSG_MOVED, STR_MSG_COPIED, STR_MSG_EMPTY_DIR,

    /* ---- 11 号 ---- */
    STR_CMD_DEL_DESC, STR_CMD_CHANGELAN_DESC, STR_CMD_VIEW_DESC,
    STR_CMD_OPEN_DESC, STR_CMD_BIN_DESC, STR_CMD_BA_DESC,
    STR_CMD_TIME_DESC, STR_CMD_CHECK_DESC, STR_CMD_PATH_DESC,
    STR_CMD_SLEEP_DESC, STR_CMD_CLOSE_DESC,

    STR_MSG_DELETED, STR_MSG_RESET_DONE,
    STR_MSG_LINE_OOR, STR_MSG_FILE_EMPTY,
    STR_MSG_OPEN_RUNNING,
    STR_MSG_BA_DONE,
    STR_MSG_PATH_ADDED,
    STR_MSG_WAKEUP,
    STR_MSG_SHUTDOWN, STR_MSG_SHUTDOWN_FAILED,
    STR_MSG_CHECK_HEADER,
        /* ---- 13 号 ---- */
    STR_MSG_QA_RUNNING,
    STR_MSG_QA_DONE,
    STR_MSG_QA_NOT_QA,
    STR_MSG_QA_ABORTED,

    STR_COUNT
} str_id_t;

void         i18n_init(void);
void         i18n_set(lang_id_t id);
lang_id_t    i18n_get(void);
const char*  i18n_str(str_id_t id);
lang_id_t    i18n_parse(const char* code);
const char*  i18n_code(lang_id_t id);
const char*  i18n_name(lang_id_t id);

#endif
