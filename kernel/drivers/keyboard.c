/* ============================================================
 * YUAOS - PS/2 键盘驱动实现
 * 文件 : kernel/drivers/keyboard.c
 * 说明 : IRQ1 中断接收扫描码，转 ASCII 或特殊键码塞环形缓冲；
 *        供 shell 阻塞式读取。
 * ============================================================ */
#include "drivers/keyboard.h"
#include "arch/cpu.h"
#include "lib/io.h"

#define KB_DATA   0x60

#define KB_BUF_SIZE 256
static volatile u8  kb_buf [KB_BUF_SIZE];
static volatile u32 kb_head = 0;
static volatile u32 kb_tail = 0;

/* 修饰键状态 */
static u8 s_ext   = 0;
static u8 s_shift = 0;
static u8 s_ctrl  = 0;
static u8 s_alt   = 0;
static u8 s_caps  = 0;

/* set 1 扫描码表（0 表示忽略） */
static const char sc_normal[128] = {
/*00*/  0, 27,'1','2','3','4','5','6','7','8','9','0','-','=','\b','\t',
/*10*/ 'q','w','e','r','t','y','u','i','o','p','[',']','\n',  0,'a','s',
/*20*/ 'd','f','g','h','j','k','l',';','\'','`',  0,'\\','z','x','c','v',
/*30*/ 'b','n','m',',','.','/',  0,'*',  0,' ',
/*3A*/  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
/*4A*/  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
/*5A*/  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
/*6A*/  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
/*7A*/  0,  0,  0,  0,  0,  0
};

static const char sc_shift[128] = {
/*00*/  0, 27,'!','@','#','$','%','^','&','*','(',')','_','+','\b','\t',
/*10*/ 'Q','W','E','R','T','Y','U','I','O','P','{','}','\n',  0,'A','S',
/*20*/ 'D','F','G','H','J','K','L',':','"','~',  0,'|','Z','X','C','V',
/*30*/ 'B','N','M','<','>','?',  0,'*',  0,' ',
/*3A*/  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
/*4A*/  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
/*5A*/  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
/*6A*/  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
/*7A*/  0,  0,  0,  0,  0,  0
};

static void kb_push(u8 c)
{
    u32 nh = (kb_head + 1) % KB_BUF_SIZE;
    if (nh == kb_tail) return;      /* 满则丢弃 */
    kb_buf[kb_head] = c;
    kb_head = nh;
}

static void kb_handle(u8 sc)
{
    if (sc == 0xE0) { s_ext = 1; return; }

    u8 pressed = (sc & 0x80) ? 0 : 1;
    u8 code    = sc & 0x7F;

    /* 修饰键 */
    if (code == 0x2A || code == 0x36) { s_shift = pressed; s_ext = 0; return; }
    if (code == 0x1D)                 { s_ctrl  = pressed; s_ext = 0; return; }
    if (code == 0x38)                 { s_alt   = pressed; s_ext = 0; return; }
    if (code == 0x3A && pressed)      { s_caps ^= 1;       s_ext = 0; return; }

    if (!pressed) { s_ext = 0; return; }

    /* 扩展键 */
    if (s_ext) {
        s_ext = 0;
        switch (code) {
        case 0x48: kb_push(KB_KEY_UP);     return;
        case 0x50: kb_push(KB_KEY_DOWN);   return;
        case 0x4B: kb_push(KB_KEY_LEFT);   return;
        case 0x4D: kb_push(KB_KEY_RIGHT);  return;
        case 0x47: kb_push(KB_KEY_HOME);   return;
        case 0x4F: kb_push(KB_KEY_END);    return;
        case 0x53: kb_push(KB_KEY_DELETE); return;
        default: return;
        }
    }

    char c = s_shift ? sc_shift[code] : sc_normal[code];
    if (!c) return;

    /* Caps Lock 仅影响字母 */
    if (s_caps && c >= 'a' && c <= 'z' && !s_shift) c -= 32;
    if (s_caps && c >= 'A' && c <= 'Z' &&  s_shift) c += 32;

    kb_push((u8)c);
}

void kb_irq_handler(regs_t* r)
{
    (void)r;
    u8 sc = inb(KB_DATA);
    kb_handle(sc);
}

void kb_init(void)
{
    kb_head = kb_tail = 0;
    s_ext = s_shift = s_ctrl = s_alt = s_caps = 0;
}

int kb_try_getchar(void)
{
    if (kb_head == kb_tail) return -1;
    u8 c = kb_buf[kb_tail];
    kb_tail = (kb_tail + 1) % KB_BUF_SIZE;
    return (int)c;
}

int kb_wait_getchar(void)
{
    for (;;) {
        int c = kb_try_getchar();
        if (c >= 0) return c;
        cpu_hlt();
    }
}
