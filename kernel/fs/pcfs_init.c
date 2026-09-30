/* ============================================================
 * YUAOS - PCFS 默认文件树
 * 文件 : kernel/fs/pcfs_init.c
 * ============================================================ */
#include "fs/pcfs.h"
#include "lib/string.h"

static fs_node_t* mkdir(fs_node_t* parent, const char* name)
{
    return fs_create_child(parent, name, FS_NODE_DIR);
}

static void mkfile(fs_node_t* parent, const char* name,
                   const void* data, u32 len)
{
    fs_node_t* f = fs_create_child(parent, name, FS_NODE_FILE);
    if (f) fs_write_file(f, data, len);
}

static void mkfile_str(fs_node_t* parent, const char* name, const char* text)
{
    mkfile(parent, name, text, (u32)strlen(text));
}

/* ============================================================
 * hello.qa —— 手工汇编的可执行文件
 *
 * 头 24 字节：
 *   magic='QAEX'  version=1  entry=0  code_size=64  data_size=0  reserved=0
 *
 * buf 内布局（buf 起点是代码段起点，不含头）：
 *   偏移 0  : 8B 44 24 04   mov  eax, [esp+4]    ; eax = rt
 *   偏移 4  : 8B 4C 24 08   mov  ecx, [esp+8]    ; ecx = buf
 *   偏移 8  : 83 C1 20      add  ecx, 32         ; ecx = buf+32 = &msg
 *   偏移 11 : 51            push ecx             ; arg
 *   偏移 12 : FF 10         call [eax]           ; rt->print(msg)
 *   偏移 14 : 83 C4 04      add  esp, 4          ; 清 cdecl 栈参数
 *   偏移 17 : C3            ret
 *   偏移 18 : (14 字节 0x90 填充，对齐到 32)
 *   偏移 32 : "Hello from .qa!\0" (16 字节)
 *   偏移 48 : (16 字节 0x90 填充，凑满 64)
 * 数组总长 = 24 + 64 = 88 字节
 * ============================================================ */
static const u8 qa_hello[] = {
    /* ---------------- 头 24 字节 ---------------- */
    'Q','A','E','X',
    0x01,0x00,0x00,0x00,             /* version   = 1 */
    0x00,0x00,0x00,0x00,             /* entry     = 0 */
    0x40,0x00,0x00,0x00,             /* code_size = 64 */
    0x00,0x00,0x00,0x00,             /* data_size = 0 */
    0x00,0x00,0x00,0x00,             /* reserved  = 0 */

    /* ---------------- 代码 18 字节 ---------------- */
    0x8B,0x44,0x24,0x04,             /* mov eax, [esp+4]      ; rt */
    0x8B,0x4C,0x24,0x08,             /* mov ecx, [esp+8]      ; buf */
    0x83,0xC1,0x20,                  /* add ecx, 32           ; &msg */
    0x51,                            /* push ecx              ; arg */
    0xFF,0x10,                       /* call [eax]            ; print */
    0x83,0xC4,0x04,                  /* add esp, 4            ; cleanup */
    0xC3,                            /* ret */

    /* ---------------- 填充到偏移 32（14 字节） ---------------- */
    0x90,0x90,0x90,0x90,0x90,0x90,0x90,
    0x90,0x90,0x90,0x90,0x90,0x90,0x90,

        /* ---------------- 字符串 @ 偏移 32（17 字节：15 字符 + '\n' + '\0'） ---------------- */
    'H','e','l','l','o',' ',
    'f','r','o','m',' ','.','q','a','!', '\n', 0x00,

    /* ---------------- 填充到偏移 64（15 字节） ---------------- */
    0x90,0x90,0x90,0x90,0x90,
    0x90,0x90,0x90,0x90,0x90,
    0x90,0x90,0x90,0x90,0x90,
};

void pcfs_build_default_tree(void)
{
    fs_node_t* root = fs_root();
    if (!root) return;

    fs_node_t* bin    = mkdir(root, "bin");
    fs_node_t* etc    = mkdir(root, "etc");
    fs_node_t* system = mkdir(root, "system");
    if (system) mkdir(system, "fonts");       /* ← 加这个 */
    mkdir(root, "tmp");
    fs_node_t* home = mkdir(root, "home");
    fs_node_t* yuaos_home = home ? mkdir(home, "yuaos") : NULL;

    /* ---- /etc ---- */
    if (etc) {
        mkfile_str(etc, "motd.ou",
               "Welcome to YUAOS!\n"
               "This OS is handcrafted, not based on any existing OS.\n"
               "Type 'help' to see the command list.\n");

        mkfile_str(etc, "version.ou",
               "YUAOS v1.0\n"
               "arch  : i686\n"
               "boot  : BIOS + stage2\n"
               "loader: custom ELF32 -> flat binary\n"
               "tool  : i686-elf-gcc + nasm\n");

        mkfile_str(etc, "path.ou", "/bin:/usr/bin\n");
    }

    /* ---- /bin ---- */
    if (bin) {
        mkfile_str(bin, "hello.set",
               "echo Hello from a .set script!\n"
               "echo YUAOS scripts are alive.\n");

        /* 真正的可执行文件 */
        mkfile(bin, "hello.qa", qa_hello, sizeof(qa_hello));

        mkfile_str(bin, "empty.set", "");
    }

    /* ---- /home/yuaos ---- */
    if (yuaos_home) {
        mkfile_str(yuaos_home, "readme.ou",
               "This is your home directory.\n"
               "Try: view readme.ou\n"
               "     list /\n"
               "     open /bin/hello.qa\n");
    }
}
