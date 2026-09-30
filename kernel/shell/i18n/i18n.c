/* ============================================================
 * YUAOS - i18n 实现
 * 文件 : kernel/shell/i18n/i18n.c
 * ============================================================ */
#include "shell/i18n/i18n.h"
#include "lib/string.h"

/* 由 langs.c 提供 */
extern const char* const g_lang_strings[LANG_COUNT][STR_COUNT];
extern const char* const g_lang_codes  [LANG_COUNT];
extern const char* const g_lang_names  [LANG_COUNT];

static lang_id_t g_current = LANG_EN;

void i18n_init(void) { g_current = LANG_EN; }

void i18n_set(lang_id_t id)
{
    if (id < LANG_COUNT) g_current = id;
}

lang_id_t i18n_get(void) { return g_current; }

const char* i18n_str(str_id_t id)
{
    if (id >= STR_COUNT) return "?";
    const char* s = g_lang_strings[g_current][id];
    return s ? s : "?";
}

lang_id_t i18n_parse(const char* code)
{
    if (!code) return LANG_COUNT;
    for (lang_id_t i = 0; i < LANG_COUNT; i++) {
        if (strcmp(g_lang_codes[i], code) == 0) return i;
    }
    return LANG_COUNT;
}

const char* i18n_code(lang_id_t id) { return (id < LANG_COUNT) ? g_lang_codes[id] : "??"; }
const char* i18n_name(lang_id_t id) { return (id < LANG_COUNT) ? g_lang_names[id] : "Unknown"; }
