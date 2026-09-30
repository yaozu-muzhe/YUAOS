# YUAOS

**纯手搓的 i686 操作系统 · v1.0**

不基于 Linux，不基于 CoolPotOS，不基于任何已有操作系统。从引导扇区到图形渲染，全部自己写。

> ⚠️ v1.0 是可运行的教学性质操作系统，不是日常可用系统。

---

## 特性

| 模块 | 状态 |
|------|------|
| 引导链 | BIOS → stage2 → VBE → 保护模式 → 内核 |
| 中断 | GDT / IDT / ISR / IRQ / PIC / PIT / RTC |
| 内存 | E820 探测 / 位图 PMM / 链表堆 |
| 文件系统 | PCFS（内存树 + ATA 持久化） |
| Shell | 40+ 命令 / 6 语言 / 脚本执行 |
| 可执行格式 | `.qa`（自定义 ELF-like 格式）+ 加载器 |
| 任务隔离 | 独立栈 + `setjmp/longjmp`，崩溃不拖垮内核 |
| 图形 | VBE 800×600×32 / 帧缓冲 / 内嵌 8×16 字库 |

---

## 环境依赖

| 工具 | 版本 | 说明 |
|------|------|------|
| NASM | ≥ 2.15 | 汇编 |
| i686-elf-gcc | ≥ 13 | i686 交叉编译器 |
| i686-elf-ld | 同上 | 链接器 |
| i686-elf-objcopy | 同上 | ELF → flat binary |
| GCC | ≥ 13 | 用于 host 侧工具（`mkfs_pcfs`） |
| QEMU | ≥ 8.0 | `qemu-system-i386` |

### Windows 安装（MSYS2 推荐）

```bash
# MSYS2 UCRT64 环境
pacman -S mingw-w64-ucrt-x86_64-nasm
pacman -S mingw-w64-ucrt-x86_64-gcc
# i686-elf-tools 需另装（见下方链接）
```

- i686-elf 交叉工具链：`https://github.com/lordmilko/i686-elf-tools`
- QEMU Windows：`https://qemu.weilnetz.de/w64/`

装好后把 `bin` 目录加入 PATH，确认：

```
nasm -v
i686-elf-gcc --version
gcc --version
qemu-system-i386 --version
```

---

## 编译 & 运行

```
build.bat
run.bat
```

`build.bat` 会：

1. 汇编引导扇区与 stage2
2. 编译内核（C + ASM）
3. 编译 `userland/apps/*/*.asm` → `.qa`
4. 运行 `mkfs_pcfs` 生成 `pcfs.img`
5. 组装 1.44MB 软盘镜像 `yuaos.img`

`run.bat`：

```
qemu-system-i386 -fda build\yuaos.img -hda build\pcfs.img -boot a
```

---

## 目录结构

```
YUAOS/
├── boot/               引导扇区 + stage2
│   ├── boot.asm
│   ├── stage2.asm
│   └── disk.inc
├── kernel/             内核
│   ├── arch/           GDT / IDT / ISR / IRQ / PIC / PIT / RTC / paging
│   ├── mm/             E820 / PMM / heap
│   ├── fs/             PCFS（内存 + 磁盘序列化）
│   ├── drivers/        VGA / FB / font_data / keyboard / ATA
│   ├── loader/         .qa 加载器
│   ├── task/           任务 / 独立栈
│   ├── shell/          Shell + 命令 + i18n
│   ├── lib/            string / printf
│   └── main.c
├── userland/
│   ├── apps/           .qa 程序源码（NASM）
│   └── libyua/         .qa runtime ABI
├── tools/
│   ├── build.ps1       构建主脚本
│   ├── mkqa.ps1        .asm → .qa
│   └── mkfs_pcfs.c     PCFS 镜像生成器
├── build.bat
├── run.bat
└── README.md
```

---

## 命令清单

Shell 命令（部分）：

```
help                 显示帮助
cls                  清屏
lang <code>          切换语言（zh jp my de en fr）
echo ...             回显
history              命令历史
about                关于
pwd                  当前路径

go to <path>         切换目录
back to [<path>]     返回目录
add <name> [in <p>]  新建文件/目录
move <n> to <path>   移动
copy <n> in <s> and add to <d>   复制
list [<path>]        列出目录
del <n> in <p>       删除
del whole            恢复出厂

view <f> [line <n>]  查看文本
open <file>          运行 .qa / .set
bin <file>           二进制转储
ba <file>            .exe→.qa，.bat→.set

time                 显示时间
check                系统状态
% <path>             追加 Path

System.sleep()       睡眠 1 秒
System.close()       关机
```

---

## 6 语言本地化

| 代码 | 语言 | 显示方式 |
|------|------|----------|
| `zh` | 中文 | 拼音 + 声调数字（`Ni3hao3`）|
| `jp` | 日本語 | 罗马字（`Konnichiwa`）|
| `my` | မြန်မာ | 拉丁转写 |
| `de` | Deutsch | 拉丁（CP437 覆盖 `ä ö ü ß`）|
| `en` | English | ASCII |
| `fr` | Français | 拉丁（`é è ç à`）|

> v1.0 未实现 CJK 点阵字库，用拉丁转写替代。真正的中日缅字形留到 v2.0。

---

## 架构设计要点

### 1. 引导

stage2 用 BIOS `int 0x10` 设置 VBE `800×600×32` LFB，然后进入保护模式，跳 `0x10000` 处的 flat binary 内核。

### 2. 内存

- E820 表由 BIOS 提供，stage2 保存到物理 `0x9000`
- PMM 位图 1 bit / 4KB 页，位于 1MB 以上
- 链表堆：first fit + 相邻块合并，启动占用 4MB

### 3. 文件系统 PCFS

- 内存树形结构（`fs_node`）
- 磁盘序列化：`TAG_DIR_BEGIN / TAG_FILE / TAG_DIR_END` 前序流
- Superblock 位于 LBA 1024，数据区 LBA 1025 起

### 4. `.qa` 可执行格式

```
偏移  大小  含义
0     4     magic "QAEX"
4     4     version = 1
8     4     entry_offset
12    4     code_size
16    4     data_size
20    4     reserved
24    ...   code + data
```

入口签名 `void _start(qa_runtime_t* rt, u8* base)`。

### 5. 任务隔离

- 每个 `.qa` 分配独立 4KB 栈页
- `setjmp/longjmp` 保存/恢复 shell 现场
- `.qa` 崩溃（如 `#PF`）触发 `isr_handler` → `task_abort` → 回 shell

---

## 已知限制

- **分页暂时禁用**：`vmm_init` 与碎片化 PMM 冲突，v1.0 关闭分页
- **无用户态**：`.qa` 跑在内核 ring 0，靠 `setjmp` 兜底
- **无多任务**：一次只跑一个任务
- **无 CJK 点阵**：中日缅用拉丁转写
- **无网络 / 声音 / 鼠标 / GUI**

---

## 从源码编 `.qa`

```
nasm -f bin -o app.bin app.asm
# 加 24 字节头 → app.qa
```

或直接用 `tools/mkqa.ps1`（build.bat 已自动跑）。

`.qa` runtime ABI：

```c
typedef struct {
    void (*print)    (const char*);
    void (*putc)     (char);
    int  (*getc)     (void);
    void (*print_hex)(u32);
    void (*sleep)    (u32);
    void (*exit)     (int);
} qa_runtime_t;
```

---

## 贡献

欢迎 issue / PR。请勿在此基础上直接基于其他操作系统开发。

---

## 协议

MIT License
