# MatrixOS 系统调用表（与代码一致）

入口：**`int 0x80`**（用户态 `DPL=3` 可触发）。

寄存器约定：

| 寄存器 | 含义 |
|--------|------|
| `EAX` | 系统调用号 |
| `EBX` | 第 1 个参数 |
| `ECX` | 第 2 个参数 |
| `EDX` | 第 3 个参数 |
| `EAX`（返回） | 返回值；失败时多为 **负数**（与 `kernel/fs/vfs.h` 中 `ENOENT` 等一致）；未实现调用见下表 |

内核实现：`kernel/syscall/syscall.c`（`syscall_handler`）、编号定义：`kernel/syscall/syscall.h`。

---

## 调用号一览

| 号 | 名称 | 参数 | 返回值 / 说明 |
|----|------|------|-----------------|
| 1 | `SYSCALL_EXIT` | `ebx` = 退出码（当前内核仅日志） | 无返回（销毁当前进程） |
| 2 | `SYSCALL_PUTS` | `ebx` = 用户空间字符串指针（教学版直接解引用） | 无 |
| 3 | `SYSCALL_GETC` | 无 | `eax` = 字符；无键为 0 |
| 4 | `SYSCALL_PUTCHAR` | `ebx` = 字符 | 无 |
| 5 | `SYSCALL_GETPID` | 无 | `eax` = 当前 PID |
| 10 | `SYSCALL_OPEN` | `ebx` = 路径串；`ecx` = flags（`O_RDONLY` 等） | `eax` = fd 或负数错误码 |
| 11 | `SYSCALL_CLOSE` | `ebx` = fd | `eax` = 0 或负数错误码 |
| 12 | `SYSCALL_READ` | `ebx` = fd；`ecx` = 缓冲区；`edx` = 长度 | `eax` = 读入字节数或错误码 |
| 13 | `SYSCALL_WRITE` | `ebx` = fd；`ecx` = 缓冲区；`edx` = 长度 | `eax` = 写出字节数或错误码 |
| 14 | `SYSCALL_SEEK` | `ebx` = fd；`ecx` = offset；`edx` = whence | `eax` = 新位置或错误码 |
| 15 | `SYSCALL_IOCTL` | `ebx` = fd；`ecx` = request；`edx` = arg 指针 | `eax` = 由设备实现决定 |
| 16 | `SYSCALL_STAT` | `ebx` = 路径；`ecx` = `struct file_stat*` | `eax` = 0 或负数错误码 |
| 其他 | — | — | `eax` = `(uint32_t)-1`（未知调用） |

---

## 用户程序中的 `equ`（须与上表一致）

- `user/programs/hello.asm`：`SYS_EXIT=1`，`SYS_PUTS=2`，`SYS_PUTCHAR=4`。
- `user/programs/file_test.asm`：含 `SYS_OPEN`…`SYS_STAT` 等；其中测试打开的文件名为 **`hello`**，须与 GRUB 模块在 RAMFS 中注册名一致（见 `include/boot_config.h` 的 `BOOT_DEMO_USER_MODULE_NAME`）。

---

## 与后续课程的关系

- 可把「未知调用」改为独立错误码并在用户态区分。
- 可把 `PUTS` 等改为先 `copy_from_user`，再内核打印（当前为教学简化直接解引用）。
