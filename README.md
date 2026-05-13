# MatrixOS

[![CI](https://github.com/BlackForest1990/MatrixOS/actions/workflows/ci.yml/badge.svg)](https://github.com/BlackForest1990/MatrixOS/actions/workflows/ci.yml)

Educational **32-bit x86 (i386)** hobby OS in **C + NASM**: Multiboot + GRUB, higher-half kernel, PMM / kmalloc / VMM, processes, `int 0x80` syscalls, VFS with RAMFS + devfs, and flat-binary user demos loaded via GRUB modules.

**中文说明见下方** — the following sections keep the original Chinese documentation.

## Quick start (Linux / macOS with deps)

### macOS（Apple Silicon 或 Intel）

Apple 自带的 `clang`/`ld` 只能生成 **Mach-O**，无法按 `link.ld` 链接本仓库的 **ELF** 内核。在 **Darwin** 上 `Makefile` 会自动改用 Homebrew 的 **`i686-elf-gcc` / `i686-elf-ld`**（不再使用 `-m32`）。

```bash
brew install nasm qemu i686-elf-gcc
eval "$(/opt/homebrew/bin/brew shellenv)"   # Intel Mac 上可能是 /usr/local/bin/brew

cd /path/to/MatrixOS
make clean && make all
make qemu           # 仅内核；串口在当前终端
make qemu-modules   # 内核 + hello/file_test 模块，无需 grub-mkrescue
```

生成 ISO（`make os.iso`）需要本机有 **`grub-mkrescue`**（macOS 上可尝试 `brew install grub`，视 Homebrew 配方而定；若困难可在 Linux 或 Docker 里打 ISO）。

### Linux（Debian / Ubuntu 等）

**依赖：** 带 **32 位** 的 `gcc`（如 `gcc-multilib`）、`nasm`、`make`、`ld`；运行需要 `qemu-system-i386`；打 ISO 需要 `grub-mkrescue`（常与 `xorriso` 等一起安装）。

```bash
make all          # kernel.elf + user/build/*.bin
make qemu         # 仅内核，串口在 stdio
make qemu-modules # 无 ISO：QEMU -initrd 加载多个模块（逗号分隔）
make os.iso       # 需要 grub-mkrescue
make qemu-iso     # ISO + GRUB 模块
```

调试：`make debug-iso`、`make debug-modules`（会 `-s -S` 等待 GDB），或 `./debug.sh` 与 `.gdbinit`。

### Boot phases & demo switch（教学用）

- 启动顺序拆在 `kernel/boot.c`（`boot_early_console`、`boot_mm_init`、…），入口只做编排：`kernel/kmain.c`。
- 常量与演示开关在 **`include/boot_config.h`**（物理内存区间、`BOOT_DEMO_USER_MODULE_NAME` / `BOOT_DEMO_USER_MODULE_NAME_2`、`BOOT_DEMO_RUN_MEMORY_TESTS` 等）。
- 最短启动（不跑 RAMFS 串口演示、不拉起用户演示进程）：`make CONFIG_DEMO_STARTUP=0 all`。
- **VFS 错误码**（`kernel/fs/vfs.h`）：均为负数；`ENOMEM` 表示 kmalloc 等分配失败，`EMFILE` 表示 fd/各 fs 私有句柄表满。
- **演示模块名**：`BOOT_DEMO_USER_MODULE_NAME` / `BOOT_DEMO_USER_MODULE_NAME_2` 与 `loader` 在无 GRUB 模块名时的回退、`test_fs` 自测一致。

## Repository layout

| Path | Role |
|------|------|
| `boot/` | Loader + GRUB config |
| `kernel/` | `boot.c`（分阶段启动）, `kmain.c`, `drivers`, `interrupts`, `mm`, `process`, `syscall`, `fs` |
| `include/`, `lib/` | `boot_config.h`、公共头与最小 libc |
| `user/programs/` | Example user ASM (`hello`, `file_test`) |
| `document/` | 中文开发总结；[`syscall-table.md`](document/syscall-table.md)（`int 0x80` 调用号与寄存器约定） |

## Contributing

Issues and PRs are welcome. Run `make all` before opening a PR; CI builds the same target on Ubuntu.

## License

Released under the [MIT License](LICENSE) unless stated otherwise in individual files.

## GitHub metadata (copy-paste)

In the repository **About** (gear icon): set **Description**, optionally **Website**, and add **Topics**.

**Suggested description**

> Educational 32-bit x86 hobby OS in C/NASM — Multiboot/GRUB, PMM/kmalloc/VMM, `int 0x80` syscalls, VFS (RAMFS/devfs), user demos under QEMU.

**Suggested topics** (paste one by one or type; GitHub suggests matching tags)

`osdev`, `operating-system`, `kernel`, `x86`, `i386`, `multiboot`, `grub`, `qemu`, `nasm`, `assembly`, `c`, `vfs`, `syscall`, `memory-management`, `bare-metal`, `hobby-os`, `educational`

---

# 自己编写操作系统


工作了很多年，其实对于底层的知识很欠缺，借助AI知识大发展，我可以很快的编写代码和定位问题，所以我就自己写了个操作系统，希望可以帮助大家对于底层知识的理解。

源码以 **MIT** 开源，见仓库根目录 [`LICENSE`](LICENSE)。

**在 macOS 上运行：** 请先 `brew install nasm qemu i686-elf-gcc`，在项目根目录执行 `make all` 后 `make qemu` 或 `make qemu-modules`（详见上方 Quick start）。演示模块名与 loader 回退名见 `include/boot_config.h`（`BOOT_DEMO_USER_MODULE_NAME*`）。

## 系统架构概览

### **分层架构设计**

1.  **硬件抽象层** - 中断处理、内存管理、设备驱动
2.  **内核核心层** - 进程管理、文件系统、系统调用
3.  **用户空间层** - 用户程序运行环境

## 核心功能模块

### **1. 内存管理子系统**

*   **伙伴系统分配器** (`pmm.c`)

    *   支持最大 8MB 的内存块分配
    *   按 2 的幂次方进行块分裂与合并
    *   位图管理和空闲链表优化
*   **内核堆分配器** (`kmalloc.c`)

    *   首次适应算法 + 最佳适应优化
    *   支持 `kmalloc/kfree/kcalloc/krealloc`
    *   内存块分裂与合并机制
*   **虚拟内存管理** (`vmm.c`)

    *   用户空间页目录创建与管理
    *   用户程序加载与地址空间设置
    *   代码段(0x00000000)和栈段(0xBFFFF000)映射
*   **临时映射框架** (`temp_mapping.c`)

    *   4个固定虚拟地址槽用于快速物理页访问
    *   引用计数和自动清理机制

### **2. 进程管理**

*   **进程控制块** (`process.h`)

    *   PID 管理、状态机(就绪/运行/终止)
    *   独立的页目录和内核栈
    *   用户栈管理(0xBFFFFFFB)
*   **用户模式切换** (`switch.asm`)

    *   通过 IRET 指令实现特权级切换
    *   用户段选择子(0x1B代码段, 0x23数据段)
    *   完整的寄存器保存与恢复

### **3. 文件系统架构**

*   **虚拟文件系统层** (`vfs.c`)

    *   统一文件操作接口(open/read/write/close/seek/ioctl/stat)
    *   文件描述符管理(32个打开文件)
    *   标准输入输出(0,1,2)重定向
*   **RAM文件系统** (`ramfs.c`)

    *   将 GRUB 模块注册为只读文件
    *   支持最大64个文件，基于内存的存储
    *   文件权限管理(0444只读模式)
*   **设备文件系统** (`devfs.c`)

    *   `/dev/console` - 控制台输入输出
    *   `/dev/null` - 空设备
    *   `/dev/zero` - 零设备

### **4. 中断与系统调用**

*   **完整的中断处理框架**

    *   256个IDT条目，支持异常和硬件中断
    *   PIC控制器重映射(0x20-0x2F)
    *   键盘中断处理(IRQ1)
*   **系统调用接口** (`syscall.c`)

    *   INT 0x80软中断，DPL=3用户可访问
    *   支持进程控制、I/O操作、文件系统调用
    *   完整的寄存器上下文保存
    *   调用号与参数约定见 [`document/syscall-table.md`](document/syscall-table.md)（须与 `kernel/syscall/syscall.h`、`user/programs/*.asm` 中 `equ` 一致）

### **5. 设备驱动**

*   **帧缓冲区驱动** (`fb.c`)

    *   VGA文本模式(80x25)，支持颜色控制
    *   硬件光标控制，屏幕滚动
    *   特殊字符处理(换行、退格、制表符)
*   **串口驱动** (`serial.c`)

    *   COM1端口(0x3F8)，支持格式化输出
    *   类似printf的`serial_printf`函数
    *   日志级别控制(DEBUG/INFO/ERROR)
*   **键盘驱动** (`keyboard.c`)

    *   PS/2键盘扫描码处理
    *   环形输入缓冲区(256字节)
    *   字符映射和修饰键处理

## 关键技术特性

### **高级内存管理**

```c
// 多级内存分配策略
物理页分配 → 伙伴系统(pmm_alloc_pages)
内核堆分配 → kmalloc/kfree  
用户空间 → vmm_create_user_space
临时访问 → temp_map_page
```

### **完整的进程隔离**

*   每个进程独立的页目录
*   用户态与内核态完全分离
*   系统调用作为唯一入口点

### **模块化文件系统**

    VFS (统一接口)
      ├── RAMFS (内存文件，只读)
      └── DEVFS (设备文件，读写)

### **健壮的错误处理**

*   系统调用错误码(ENOENT/EACCES/EMFILE等)
*   内存分配失败检测
*   中断处理安全机制

## 用户程序支持


### **系统调用API**

    ; 用户程序示例
    mov eax, SYS_PUTS    ; 系统调用号
    mov ebx, message     ; 参数
    int 0x80             ; 触发系统调用

### **多程序加载**

*   通过GRUB模块机制加载用户程序
*   支持同时加载多个用户程序
*   文件系统集成(程序作为文件访问)


对于具体的实现，请大家参考 document 文件夹里的操作系统开发总结，如果希望 sponsor 作者的，哥们也是大大的欢迎，哈哈哈哈。


<img src="https://github.com/user-attachments/assets/7cd1338d-a078-4fa1-aaab-51896d492a0e" width="30%" alt="MatrixOS">
