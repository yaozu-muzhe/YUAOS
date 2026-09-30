/* ============================================================
 * YUAOS - 任务实现
 * 文件 : kernel/task/task.c
 * ============================================================ */
#include "task/task.h"
#include "mm/pmm.h"
#include "lib/string.h"

typedef struct {
    int        active;
    char       name[32];
    u32        stack_base;      /* PMM 页起始 */
    jmp_buf_t  exit_jmp;
    int        exit_code;
    int        aborted;
    int        abort_reason;
} task_ctx_t;

static task_ctx_t g_task;

extern u32 task_call(u32 new_esp,
                     void (*fn)(void*, void*),
                     void* a, void* b);

void task_init(void)
{
    memset(&g_task, 0, sizeof(g_task));
}

int task_is_running(void)
{
    return g_task.active;
}

const char* task_current_name(void)
{
    return g_task.active ? g_task.name : "(none)";
}

int task_run(const char* name,
             void (*entry)(void*, void*),
             void* a, void* b,
             int*  out_exit)
{
    if (g_task.active) return -1;

    u32 stack = pmm_alloc_page();
    if (!stack) return -2;

    memset(&g_task, 0, sizeof(g_task));
    g_task.active     = 1;
    g_task.stack_base = stack;
    strncpy(g_task.name, name ? name : "?", 31);

    int jr = setjmp_(&g_task.exit_jmp);

    if (jr == 0) {
        /* 首次：切到任务栈并调用 entry */
        u32 new_esp = stack + PAGE_SIZE - 16;
        new_esp &= ~0xFu;
        task_call(new_esp, entry, a, b);
        /* 只有 entry 正常 ret 才会到这里 */
    }

    /* longjmp 或正常返回都汇聚到此 */
    g_task.active = 0;
    pmm_free_page(stack);

    if (out_exit) *out_exit = g_task.exit_code;
    if (g_task.aborted) return g_task.abort_reason;
    return 0;
}

void task_exit(int code)
{
    if (!g_task.active) return;
    g_task.exit_code = code;
    g_task.aborted   = 0;
    longjmp_(&g_task.exit_jmp, 1);
    /* 不会返回 */
}

int task_abort(int reason)
{
    if (!g_task.active) return -1;
    g_task.aborted      = 1;
    g_task.abort_reason = reason;
    longjmp_(&g_task.exit_jmp, 2);
    return 0;
}
