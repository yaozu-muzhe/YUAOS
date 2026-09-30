/* ============================================================
 * YUAOS - .qa 加载器实现
 * 文件 : kernel/loader/qa.c
 * ============================================================ */
#include "loader/qa.h"
#include "task/task.h"
#include "lib/string.h"
#include "lib/printf.h"
#include "arch/cpu.h"
#include "time.h"
#include "drivers/keyboard.h"
#include "mm/heap.h"

typedef struct {
    u8  magic[4];
    u32 version;
    u32 entry;
    u32 code_size;
    u32 data_size;
    u32 reserved;
} __attribute__((packed)) qa_header_t;

#define QA_HDR_SIZE  24
#define QA_CODE_MAX  (4u * 1024u * 1024u)

#define QA_CALLBACK __attribute__((cdecl, noinline))

/* ============================================================
 * runtime 回调
 * ============================================================ */
static QA_CALLBACK void qa_print(const char* s) { if (s) kprintf("%s", s); }
static QA_CALLBACK void qa_putc(char c)         { kprintf("%c", c); }
static QA_CALLBACK int  qa_getc(void)           { return kb_try_getchar(); }
static QA_CALLBACK void qa_print_hex(u32 v)     { kprintf("0x%X", v); }

static QA_CALLBACK void qa_sleep(u32 ms)
{
    u32 ticks = ms / 10;
    if (ticks == 0) ticks = 1;
    u32 start = (u32)kernel_ticks();
    while ((u32)kernel_ticks() - start < ticks) cpu_hlt();
}

/* exit 现在真正返回 task_run */
static QA_CALLBACK void qa_exit(int code)
{
    task_exit(code);
    /* 不会返回 */
}

static qa_runtime_t g_qa_rt = {
    qa_print, qa_putc, qa_getc, qa_print_hex, qa_sleep, qa_exit,
};

/* ============================================================
 * 校验
 * ============================================================ */
int qa_is_valid_file(const fs_node_t* file)
{
    if (!file) return 0;
    if (file->type != FS_NODE_FILE) return 0;
    if (file->size < QA_HDR_SIZE) return 0;
    const u8* p = (const u8*)file->data;
    return (p[0] == 'Q' && p[1] == 'A' && p[2] == 'E' && p[3] == 'X');
}

/* ============================================================
 * 加载 + 执行
 * ============================================================ */
int qa_load_and_run(const char* name, fs_node_t* file, int* out_exit)
{
    if (out_exit) *out_exit = 0;
    if (!file || file->type != FS_NODE_FILE) return -1;

    if (file->size < QA_HDR_SIZE) {
        kprintf("qa: %s: file too small\n", name);
        return -1;
    }

    const qa_header_t* h = (const qa_header_t*)file->data;

    if (h->magic[0] != 'Q' || h->magic[1] != 'A' ||
        h->magic[2] != 'E' || h->magic[3] != 'X') {
        kprintf("qa: %s: bad magic\n", name);
        return -2;
    }
    if (h->version != 1) {
        kprintf("qa: %s: unsupported version %u\n", name, h->version);
        return -3;
    }
    if (h->code_size == 0 || h->code_size > QA_CODE_MAX) {
        kprintf("qa: %s: bad code_size %u\n", name, h->code_size);
        return -4;
    }
    if (h->entry >= h->code_size) {
        kprintf("qa: %s: bad entry %u\n", name, h->entry);
        return -5;
    }
    if (QA_HDR_SIZE + h->code_size + h->data_size > file->size) {
        kprintf("qa: %s: file truncated\n", name);
        return -6;
    }

    u32 total = h->code_size + h->data_size;
    u8* buf = (u8*)kmalloc(total);
    if (!buf) {
        kprintf("qa: %s: out of memory (%u bytes)\n", name, total);
        return -7;
    }

    memcpy(buf, file->data + QA_HDR_SIZE, h->code_size);
    if (h->data_size > 0) {
        memcpy(buf + h->code_size,
               file->data + QA_HDR_SIZE + h->code_size,
               h->data_size);
    }

    typedef void (*qa_entry_fn)(qa_runtime_t*, u8*);
    qa_entry_fn fn = (qa_entry_fn)(buf + h->entry);

    int exit_code = 0;
    int r = task_run(name, (void(*)(void*,void*))fn, &g_qa_rt, buf, &exit_code);

    kfree(buf);

    if (out_exit) *out_exit = exit_code;
    return r;   /* 0 = 正常，正数 = 异常号 */
}
