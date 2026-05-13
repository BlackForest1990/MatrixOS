# MatrixOS（中文说明）

构建命令、依赖与 Quick start 以仓库根目录的 **[README.md](../README.md)** 为准；本文侧重背景、架构与模块导读。

---

# 自己编写操作系统

工作了很多年，其实对于底层的知识很欠缺，借助 AI 知识大发展，我可以很快地编写代码和定位问题，所以我就自己写了个操作系统，希望可以帮助大家对于底层知识的理解。

源码以 **MIT** 开源，见仓库根目录 [`LICENSE`](../LICENSE)。

**在 macOS 上运行：** 请先 `brew install nasm qemu i686-elf-gcc`，在项目根目录执行 `make all` 后 `make qemu` 或 `make qemu-modules`（步骤说明见 [README.md](../README.md) 的 Quick start）。演示模块名与 loader 回退名见 `include/boot_config.h`（`BOOT_DEMO_USER_MODULE_NAME*`）。

## 系统架构概览

### 分层架构设计

1. **硬件抽象层** — 中断处理、内存管理、设备驱动  
2. **内核核心层** — 进程管理、文件系统、系统调用  
3. **用户空间层** — 用户程序运行环境  

## 核心功能模块

### 1. 内存管理子系统

* **伙伴系统分配器** (`pmm.c`)
  * 支持最大 8MB 的内存块分配  
  * 按 2 的幂次方进行块分裂与合并  
  * 位图管理和空闲链表优化  
* **内核堆分配器** (`kmalloc.c`)
  * 首次适应算法 + 最佳适应优化  
  * 支持 `kmalloc` / `kfree` / `kcalloc` / `krealloc`  
  * 内存块分裂与合并机制  
* **虚拟内存管理** (`vmm.c`)
  * 用户空间页目录创建与管理  
  * 用户程序加载与地址空间设置  
  * 代码段 (0x00000000) 和栈段 (0xBFFFF000) 映射  
* **临时映射框架** (`temp_mapping.c`)
  * 4 个固定虚拟地址槽用于快速物理页访问  
  * 引用计数和自动清理机制  

### 2. 进程管理

* **进程控制块** (`process.h`)
  * PID 管理、状态机（就绪 / 运行 / 终止）  
  * 独立的页目录和内核栈  
  * 用户栈管理 (0xBFFFFFFB)  
* **用户模式切换** (`switch.asm`)
  * 通过 IRET 指令实现特权级切换  
  * 用户段选择子（0x1B 代码段，0x23 数据段）  
  * 完整的寄存器保存与恢复  

### 3. 文件系统架构

* **虚拟文件系统层** (`vfs.c`)
  * 统一文件操作接口（open / read / write / close / seek / ioctl / stat）  
  * 文件描述符管理（32 个打开文件）  
  * 标准输入输出 (0,1,2) 重定向  
* **RAM 文件系统** (`ramfs.c`)
  * 将 GRUB 模块注册为只读文件  
  * 支持最大 64 个文件，基于内存的存储  
  * 文件权限管理（0444 只读模式）  
* **设备文件系统** (`devfs.c`)
  * `/dev/console` — 控制台输入输出  
  * `/dev/null` — 空设备  
  * `/dev/zero` — 零设备  

### 4. 中断与系统调用

* **完整的中断处理框架**
  * 256 个 IDT 条目，支持异常和硬件中断  
  * PIC 控制器重映射 (0x20–0x2F)  
  * 键盘中断处理 (IRQ1)  
* **系统调用接口** (`syscall.c`)
  * INT 0x80 软中断，DPL=3 用户可访问  
  * 支持进程控制、I/O 操作、文件系统调用  
  * 完整的寄存器上下文保存  
  * 调用号与参数约定见 [`syscall-table.md`](syscall-table.md)（须与 `kernel/syscall/syscall.h`、`user/programs/*.asm` 中 `equ` 一致）  

### 5. 设备驱动

* **帧缓冲区驱动** (`fb.c`)
  * VGA 文本模式 (80×25)，支持颜色控制  
  * 硬件光标控制，屏幕滚动  
  * 特殊字符处理（换行、退格、制表符）  
* **串口驱动** (`serial.c`)
  * COM1 端口 (0x3F8)，支持格式化输出  
  * 类似 printf 的 `serial_printf` 函数  
  * 日志级别控制（DEBUG / INFO / ERROR）  
* **键盘驱动** (`keyboard.c`)
  * PS/2 键盘扫描码处理  
  * 环形输入缓冲区（256 字节）  
  * 字符映射和修饰键处理  

## 关键技术特性

### 高级内存管理

```c
// 多级内存分配策略
物理页分配 → 伙伴系统(pmm_alloc_pages)
内核堆分配 → kmalloc/kfree
用户空间 → vmm_create_user_space
临时访问 → temp_map_page
```

### 完整的进程隔离

* 每个进程独立的页目录  
* 用户态与内核态完全分离  
* 系统调用作为唯一入口点  

### 模块化文件系统

```text
VFS (统一接口)
  ├── RAMFS (内存文件，只读)
  └── DEVFS (设备文件，读写)
```

### 健壮的错误处理

* 系统调用错误码（ENOENT / EACCES / EMFILE 等）  
* 内存分配失败检测  
* 中断处理安全机制  

## 用户程序支持

### 系统调用 API

```nasm
; 用户程序示例
mov eax, SYS_PUTS    ; 系统调用号
mov ebx, message     ; 参数
int 0x80             ; 触发系统调用
```

### 多程序加载

* 通过 GRUB 模块机制加载用户程序  
* 支持同时加载多个用户程序  
* 文件系统集成（程序作为文件访问）  

---

更细的实现脉络见 **[操作系统开发总结](操作系统开发总结.md)**。若希望 sponsor 作者，同样欢迎。

<img src="https://github.com/user-attachments/assets/7cd1338d-a078-4fa1-aaab-51896d492a0e" width="30%" alt="MatrixOS">
