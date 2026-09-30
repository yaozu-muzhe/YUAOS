/* ============================================================
 * YUAOS - 内核主函数
 * 文件 : kernel/main.c
 * ============================================================ */
#include "kernel.h"
#include "types.h"
#include "log.h"
#include "time.h"
#include "shell/shell.h"
#include "task/task.h"
#include "fs/pcfs.h"
#include "fs/pcfs_disk.h"
#include "drivers/vga.h"
#include "drivers/fb.h"
#include "drivers/ata.h"
#include "arch/cpu.h"
#include "arch/regs.h"
#include "arch/gdt.h"
#include "arch/idt.h"
#include "arch/pic.h"
#include "arch/pit.h"
#include "arch/irq.h"
#include "mm/e820.h"
#include "mm/pmm.h"
#include "mm/heap.h"

volatile u64 g_kernel_ticks = 0;

static void timer_tick(regs_t* r) { (void)r; g_kernel_ticks++; }

void kernel_main(void)
{
    vga_init();
    vga_clear();

    klog_init();
    KLOG_INFO("boot: VGA + log online");

    gdt_init();     KLOG_INFO("GDT : loaded");
    idt_init();     KLOG_INFO("IDT : loaded (256 vectors)");
    pic_init();     KLOG_INFO("PIC : remapped");
    pit_init(100);  KLOG_INFO("PIT : 100 Hz");
    irq_register(0, timer_tick);
    irq_unmask(0);

    e820_dump();
    pmm_init();     KLOG_INFO("PMM : %u pages  %u free",
                              pmm_total_pages(), pmm_free_pages());
    heap_init();    KLOG_INFO("Heap: %u B total", heap_total());

    task_init();
    KLOG_INFO("Task: subsystem initialized");
    KLOG_INFO("FB  : framebuffer at 0x%X", (u32)fb_mem());

    fs_init();
    KLOG_INFO("PCFS: default tree built (%u files, %u dirs)",
              fs_count_files(), fs_count_dirs());

    if (ata_init() == 0) {
        KLOG_INFO("ATA : primary master detected");
        if (pcfs_disk_load() == 0) {
            KLOG_INFO("PCFS: loaded from disk (%u files, %u dirs)",
                      fs_count_files(), fs_count_dirs());
        } else {
            KLOG_INFO("PCFS: no valid image, formatting...");
            fs_reset();
            if (pcfs_disk_save() == 0) {
                KLOG_INFO("PCFS: initial image written");
            } else {
                KLOG_INFO("PCFS: write failed, volatile");
            }
        }
    } else {
        KLOG_INFO("ATA : no device, PCFS volatile");
    }

    cpu_sti();
    KLOG_INFO("Interrupts enabled. Starting shell.");

    shell_init();
    shell_run();
}
