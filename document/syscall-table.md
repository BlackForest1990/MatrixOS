# MatrixOS system call table (matches the code)

Entry: **`int 0x80`** (callable from user mode with **DPL=3**).

Register convention:

| Register | Role |
|----------|------|
| `EAX` | System call number |
| `EBX` | First argument |
| `ECX` | Second argument |
| `EDX` | Third argument |
| `EAX` (return) | Return value; errors are usually **negative** (same convention as `kernel/fs/vfs.h`, e.g. `ENOENT`). Unknown calls: see table. |

Implementation: `kernel/syscall/syscall.c` (`syscall_handler`), numbers in `kernel/syscall/syscall.h`.

---

## Call numbers

| # | Name | Arguments | Return / notes |
|---|------|-------------|----------------|
| 1 | `SYSCALL_EXIT` | `ebx` = exit code (kernel currently logs) | No return (destroys current process) |
| 2 | `SYSCALL_PUTS` | `ebx` = user string pointer (teaching build: direct dereference) | — |
| 3 | `SYSCALL_GETC` | — | `eax` = character, or `0` if none |
| 4 | `SYSCALL_PUTCHAR` | `ebx` = character | — |
| 5 | `SYSCALL_GETPID` | — | `eax` = current PID |
| 10 | `SYSCALL_OPEN` | `ebx` = path; `ecx` = flags (`O_RDONLY`, …) | `eax` = fd or negative errno |
| 11 | `SYSCALL_CLOSE` | `ebx` = fd | `eax` = `0` or negative errno |
| 12 | `SYSCALL_READ` | `ebx` = fd; `ecx` = buffer; `edx` = length | `eax` = bytes read or errno |
| 13 | `SYSCALL_WRITE` | `ebx` = fd; `ecx` = buffer; `edx` = length | `eax` = bytes written or errno |
| 14 | `SYSCALL_SEEK` | `ebx` = fd; `ecx` = offset; `edx` = whence | `eax` = new offset or errno |
| 15 | `SYSCALL_IOCTL` | `ebx` = fd; `ecx` = request; `edx` = arg pointer | `eax` = device-specific |
| 16 | `SYSCALL_STAT` | `ebx` = path; `ecx` = `struct file_stat*` | `eax` = `0` or negative errno |
| other | — | — | `eax` = `(uint32_t)-1` (unknown syscall) |

---

## `equ` in user programs (must match this table)

- `user/programs/hello.asm`: `SYS_EXIT=1`, `SYS_PUTS=2`, `SYS_PUTCHAR=4`.
- `user/programs/file_test.asm`: `SYS_OPEN` … `SYS_STAT`, etc. The test opens a file named **`hello`**, which must match the GRUB module name registered in RAMFS (see `BOOT_DEMO_USER_MODULE_NAME` in `include/boot_config.h`).

---

## Possible follow-ups (teaching)

- Use a dedicated errno for unknown syscalls instead of `(uint32_t)-1`.
- Add `copy_from_user` before kernel uses pointers like `SYSCALL_PUTS` (current code is simplified for teaching).
