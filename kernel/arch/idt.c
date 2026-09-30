/* ============================================================
 * YUAOS - IDT 实现
 * 文件 : kernel/arch/idt.c
 * 说明 : 256 个门；0-31 异常、32-47 IRQ
 * ============================================================ */
#include "arch/idt.h"
#include "lib/string.h"

typedef struct {
    u16 base_lo;
    u16 sel;
    u8  always0;
    u8  flags;
    u16 base_hi;
} __attribute__((packed)) idt_entry_t;

typedef struct {
    u16 limit;
    u32 base;
} __attribute__((packed)) idt_ptr_t;

#define IDT_ENTRIES 256

static idt_entry_t idt_entries[IDT_ENTRIES];
static idt_ptr_t   idt_ptr;

/* ---- 由 isr_stub.asm 导出：异常 ---- */
extern void isr0(void);  extern void isr1(void);  extern void isr2(void);  extern void isr3(void);
extern void isr4(void);  extern void isr5(void);  extern void isr6(void);  extern void isr7(void);
extern void isr8(void);  extern void isr9(void);  extern void isr10(void); extern void isr11(void);
extern void isr12(void); extern void isr13(void); extern void isr14(void); extern void isr15(void);
extern void isr16(void); extern void isr17(void); extern void isr18(void); extern void isr19(void);
extern void isr20(void); extern void isr21(void); extern void isr22(void); extern void isr23(void);
extern void isr24(void); extern void isr25(void); extern void isr26(void); extern void isr27(void);
extern void isr28(void); extern void isr29(void); extern void isr30(void); extern void isr31(void);

/* ---- 由 isr_stub.asm 导出：IRQ ---- */
extern void irq0(void);  extern void irq1(void);  extern void irq2(void);  extern void irq3(void);
extern void irq4(void);  extern void irq5(void);  extern void irq6(void);  extern void irq7(void);
extern void irq8(void);  extern void irq9(void);  extern void irq10(void); extern void irq11(void);
extern void irq12(void); extern void irq13(void); extern void irq14(void); extern void irq15(void);

/* 标志位：P=1 DPL=00 0 0 1 1 1 0 => 0x8E（32 位中断门） */
#define IDT_FLAG_INT_GATE  0x8E

void idt_set_gate(u8 num, u32 base, u16 sel, u8 flags)
{
    idt_entries[num].base_lo = (u16)(base & 0xFFFF);
    idt_entries[num].base_hi = (u16)((base >> 16) & 0xFFFF);
    idt_entries[num].sel     = sel;
    idt_entries[num].always0 = 0;
    idt_entries[num].flags   = flags;
}

static inline void idt_load(void)
{
    __asm__ volatile ("lidt (%0)" : : "r"(&idt_ptr));
}

#define SET_ISR(n) idt_set_gate(n,       (u32)isr##n, 0x08, IDT_FLAG_INT_GATE)
#define SET_IRQ(n) idt_set_gate(32 + (n),(u32)irq##n, 0x08, IDT_FLAG_INT_GATE)

void idt_init(void)
{
    idt_ptr.limit = (u16)(sizeof(idt_entries) - 1);
    idt_ptr.base  = (u32)&idt_entries;

    memset(idt_entries, 0, sizeof(idt_entries));

    SET_ISR(0);  SET_ISR(1);  SET_ISR(2);  SET_ISR(3);
    SET_ISR(4);  SET_ISR(5);  SET_ISR(6);  SET_ISR(7);
    SET_ISR(8);  SET_ISR(9);  SET_ISR(10); SET_ISR(11);
    SET_ISR(12); SET_ISR(13); SET_ISR(14); SET_ISR(15);
    SET_ISR(16); SET_ISR(17); SET_ISR(18); SET_ISR(19);
    SET_ISR(20); SET_ISR(21); SET_ISR(22); SET_ISR(23);
    SET_ISR(24); SET_ISR(25); SET_ISR(26); SET_ISR(27);
    SET_ISR(28); SET_ISR(29); SET_ISR(30); SET_ISR(31);

    SET_IRQ(0);  SET_IRQ(1);  SET_IRQ(2);  SET_IRQ(3);
    SET_IRQ(4);  SET_IRQ(5);  SET_IRQ(6);  SET_IRQ(7);
    SET_IRQ(8);  SET_IRQ(9);  SET_IRQ(10); SET_IRQ(11);
    SET_IRQ(12); SET_IRQ(13); SET_IRQ(14); SET_IRQ(15);

    idt_load();
}
