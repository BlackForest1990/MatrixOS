# MatrixOS

[![CI](https://github.com/BlackForest1990/MatrixOS/actions/workflows/ci.yml/badge.svg)](https://github.com/BlackForest1990/MatrixOS/actions/workflows/ci.yml)

Educational **32-bit x86 (i386)** hobby OS in **C + NASM**: Multiboot + GRUB, higher-half kernel, PMM / kmalloc / VMM, processes, `int 0x80` syscalls, VFS with RAMFS + devfs, and flat-binary user demos loaded via GRUB modules.

Long-form design notes: **[document/os-development-summary.md](document/os-development-summary.md)** · Syscall reference: **[document/syscall-table.md](document/syscall-table.md)** · **Flow charts:** **[document/flow-overview.md](document/flow-overview.md)**

## Quick start (Linux / macOS)

### macOS (Apple Silicon or Intel)

Apple’s system `clang` / `ld` produce **Mach-O**, not the **ELF** kernel this repo links with `link.ld`. On **Darwin**, the `Makefile` uses Homebrew **`i686-elf-gcc`** and **`i686-elf-ld`** (no `-m32`).

```bash
brew install nasm qemu i686-elf-gcc
eval "$(/opt/homebrew/bin/brew shellenv)"   # Intel Mac: often /usr/local/bin/brew

cd /path/to/MatrixOS
make clean && make all
make qemu           # kernel only; serial on this terminal
make qemu-modules   # kernel + hello/file_test modules; no grub-mkrescue
```

`make os.iso` needs **`grub-mkrescue`** on the machine (`brew install grub` may work depending on the formula; otherwise build the ISO on Linux or in Docker).

### Linux (Debian / Ubuntu, etc.)

**Dependencies:** `gcc` with **32-bit** support (e.g. `gcc-multilib`), `nasm`, `make`, `ld`; run with `qemu-system-i386`; ISO needs `grub-mkrescue` (often pulled in with `xorriso`).

```bash
make all          # kernel.elf + user/build/*.bin
make qemu         # kernel only; serial on stdio
make qemu-modules # no ISO: QEMU -initrd with comma-separated modules
make os.iso       # needs grub-mkrescue
make qemu-iso     # ISO + GRUB modules
```

Debug: `make debug-iso`, `make debug-modules` (QEMU `-s -S` for GDB), or `./debug.sh` with `.gdbinit`.

### Boot phases and demo switches (teaching build)

- Boot stages live in `kernel/boot.c` (`boot_early_console`, `boot_mm_init`, …); `kernel/kmain.c` only orchestrates.
- Constants and demo toggles: **`include/boot_config.h`** (physical RAM layout, `BOOT_DEMO_USER_MODULE_NAME` / `BOOT_DEMO_USER_MODULE_NAME_2`, `BOOT_DEMO_RUN_MEMORY_TESTS`, …).
- Minimal startup (skip RAMFS serial demo and user demo process): `make CONFIG_DEMO_STARTUP=0 all`.
- **VFS error codes** (`kernel/fs/vfs.h`): negative values; `ENOMEM` for kmalloc-style failures, `EMFILE` when fd / per-fs private handle tables are full.
- **Demo module names:** `BOOT_DEMO_USER_MODULE_NAME` / `BOOT_DEMO_USER_MODULE_NAME_2` match the loader fallback when GRUB does not pass a module name, and `test_fs` self-checks.

## Repository layout

| Path | Role |
|------|------|
| `boot/` | Loader + GRUB config |
| `kernel/` | `boot.c` (staged boot), `kmain.c`, `drivers`, `interrupts`, `mm`, `process`, `syscall`, `fs` |
| `include/`, `lib/` | `boot_config.h`, shared headers, minimal libc |
| `user/programs/` | Example user ASM (`hello`, `file_test`) |
| `document/` | [OS development notes](document/os-development-summary.md), [syscall table](document/syscall-table.md), [overall flow (Mermaid)](document/flow-overview.md) |

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
