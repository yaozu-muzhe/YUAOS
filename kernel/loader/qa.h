/* ============================================================
 * YUAOS - .qa 加载器
 * 文件 : kernel/loader/qa.h
 * ============================================================ */
#ifndef YUAOS_LOADER_QA_H
#define YUAOS_LOADER_QA_H

#include "types.h"
#include "fs/pcfs.h"

typedef struct qa_runtime {
    void (*print)    (const char* s);
    void (*putc)     (char c);
    int  (*getc)     (void);
    void (*print_hex)(u32 v);
    void (*sleep)    (u32 ms);
    void (*exit)     (int code);
} qa_runtime_t;

/* 加载并运行 .qa
 *   返回 0    = 正常退出（*out_exit 有效）
 *   返回 正数 = 异常中止（值为异常号，如 14 = #PF）
 *   返回 负数 = 加载/校验失败 */
int qa_load_and_run(const char* name, fs_node_t* file, int* out_exit);

int qa_is_valid_file(const fs_node_t* file);

#endif
