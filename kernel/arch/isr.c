/* ============================================================
 * YUAOS - 异常分发（含任务隔离）
 * 文件 : kernel/arch/isr.c
 * ============================================================ */
#include "arch/isr.h"
#include "arch/cpu.h"
#include "task/task.h"
#include "lib/printf.h"

static const char* g_exc_names[32] = {
    "Divide-by-zero", "Debug", "Non-maskable interrupt", "Breakpoint",
    "Overflow", "Bound range exceeded", "Invalid opcode", "Device not available",
    "Double fault", "Coprocessor segment overrun", "Invalid TSS",
    "Segment not present", "Stack-segment fault", "General protection fault",
    "Page fault", "Reserved", "x87 floating-point exception",
    "Alignment check", "Machine check", "SIMD floating-point exception",
    "Virtualization exception", "Control protection exception",
    "Reserved", "Reserved", "Reserved", "Reserved",
    "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved"
};

const char* exception_name(u32 n)
{
    return (n < 32) ? g_exc_names[n] : "Unknown";
}

void isr_handler(regs_t* r)
{
    /* ---- 任务内部异常：中止任务，回到 shell ---- */
    if (task_is_running()) {
        kprintf("\n[task:%s] Exception %u : %s\n",
                task_current_name(), r->int_no, exception_name(r->int_no));
        kprintf("  eip=%p  cs=0x%X  err=0x%X\n",
                (void*)r->eip, r->cs, r->err_code);
        kprintf("  task aborted, returning to shell.\n\n");
        task_abort((int)r->int_no);
        /* 不会返回 */
    }

    /* ---- 内核态异常：真的挂了 ---- */
    kprintf("\n");
    kprintf("Exception %u : %s\n", r->int_no, exception_name(r->int_no));
    kprintf("  err_code = 0x%X\n", r->err_code);
    kprintf("  eip      = 0x%X\n", r->eip);
    kprintf("  cs       = 0x%X  eflags = 0x%X\n", r->cs, r->eflags);
    kprintf("  eax=%X ebx=%X ecx=%X edx=%X\n", r->eax, r->ebx, r->ecx, r->edx);
    kprintf("  esi=%X edi=%X ebp=%X\n", r->esi, r->edi, r->ebp);
    kprintf("System halted.\n");

    cpu_halt_forever();
}
