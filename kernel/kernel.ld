/* ============================================================
 * YUAOS - 内核链接脚本
 * 文件 : kernel/kernel.ld
 * 目标 : ELF32 i386，加载到物理 0x10000，转为 flat binary
 * ============================================================ */

OUTPUT_FORMAT(elf32-i386)
OUTPUT_ARCH(i386)
ENTRY(_start)

SECTIONS
{
    . = 0x10000;

    .text : ALIGN(4) {
        *(.text.entry)
        *(.text*)
    }

    .rodata : ALIGN(4) {
        *(.rodata*)
    }

    .data : ALIGN(4) {
        *(.data*)
    }

    __bss_start = .;
    .bss : ALIGN(4) {
        *(.bss*)
        *(COMMON)
    }
    __bss_end = .;

    _kernel_end = .;

    /DISCARD/ : {
        *(.comment)
        *(.note*)
        *(.eh_frame*)
    }
}
