/* ============================================================
 * YUAOS - 任务 / 独立栈
 * 文件 : kernel/task/task.h
 * ============================================================ */
#ifndef YUAOS_TASK_TASK_H
#define YUAOS_TASK_TASK_H

#include "types.h"

/* 简化版 jmp_buf：保存 callee-saved 寄存器 + esp/eip/eflags */
typedef struct {
    u32 ebx, esi, edi, ebp;
    u32 esp;
    u32 eip;
    u32 eflags;
} jmp_buf_t;

int  setjmp_(jmp_buf_t* env);
void longjmp_(jmp_buf_t* env, int val);

void task_init(void);

/* 运行一个任务并等待其结束。
 *   entry(arg1, arg2) 将在独立内核栈上执行
 *   返回 0        = 正常退出（*out_exit 有效）
 *   返回 非 0     = 异常中止（返回异常号） */
int  task_run(const char* name,
              void (*entry)(void*, void*),
              void* a, void* b,
              int*  out_exit);

/* 在任务内部调用：结束任务并返回调度器（不返回） */
void task_exit(int code);

/* 从异常处理调用：中止当前任务并返回调度器 */
int  task_abort(int reason);

int  task_is_running(void);
const char* task_current_name(void);

#endif
