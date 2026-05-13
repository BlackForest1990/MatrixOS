# MatrixOS — overall flow (reference)

This document captures the **main control flow** of the teaching kernel: from GRUB to `kmain`, through staged `boot_*` helpers in `kernel/boot.c`, then runtime entry paths. Source of truth for ordering: **`kernel/kmain.c`**, **`kernel/boot.c`**, **`boot/loader.asm`**.

Diagrams use [Mermaid](https://mermaid.js.org/). They render on GitHub and in many Markdown preview tools.

---

## 1. Boot chain (firmware → kernel entry)

GRUB loads the kernel as a **Multiboot** image. The assembly entry in `boot/loader.asm` sets up a minimal 32-bit environment (stack, BSS, higher-half mapping as implemented there), then calls **`kmain(uint32_t magic, uint32_t addr)`** with the Multiboot magic and **physical** pointer to the Multiboot information structure.

```mermaid
flowchart TB
  subgraph firmware [Bootloader]
    GRUB[GRUB Multiboot]
  end
  subgraph loader_asm [boot/loader.asm]
    ASM[32-bit entry: stack, BSS, paging setup]
    CALL["call kmain(magic, mbi_phys)"]
  end
  subgraph entry_c [kernel/kmain.c]
    KM[kmain]
  end
  GRUB --> ASM --> CALL --> KM
```

---

## 2. `kmain` orchestration (stages and optional demos)

`kmain` only **sequences** calls; each step is implemented in **`kernel/boot.c`**. Optional steps are gated by macros in **`include/boot_config.h`** (e.g. `BOOT_DEMO_RUN_FILESYSTEM_TESTS`, `BOOT_DEMO_RUN_MEMORY_TESTS`, `BOOT_DEMO_RUN_USER_PROGRAM`).

```mermaid
flowchart TD
  A["kmain(magic, addr)"] --> B["boot_early_console() — fb_init, serial"]
  B --> C{"boot_multiboot_ok(magic)?"}
  C -->|no| Z["return"]
  C -->|yes| D["boot_mm_init() — pmm_init, temp_mapping_init, kmalloc_init"]
  D --> E["boot_kernel_subsystems_init() — process_init, syscall_init"]
  E --> F["boot_modules_from_mbi(mbi) — loader_init from Multiboot modules"]
  F --> G["boot_vfs_init() — vfs_init, ramfs_init, devfs_init"]
  G --> H{"BOOT_DEMO_RUN_FILESYSTEM_TESTS?"}
  H -->|yes| I["boot_filesystem_demo()"]
  H -->|no| J["skip"]
  I --> K
  J --> K["boot_interrupts_and_input() — isr_install, STI, keyboard_init"]
  K --> L["boot_print_ready()"]
  L --> M{"BOOT_DEMO_RUN_MEMORY_TESTS?"}
  M -->|yes| N["boot_optional_memory_tests()"]
  M -->|no| O["skip"]
  N --> P
  O --> P{"BOOT_DEMO_RUN_USER_PROGRAM?"}
  P -->|yes| Q["boot_user_demo() — stat module, process_create_from_module, process_start"]
  P -->|no| R["skip"]
  Q --> S
  R --> S["boot_idle_forever() — poll keyboard, HLT loop"]
```

**Notes**

- **Memory first:** PMM → temp mapping → `kmalloc`, so later VFS and PCB allocations can succeed.
- **Modules before RAMFS:** `loader_init` records GRUB modules; **RAMFS** exposes them as read-only files (names must match `BOOT_DEMO_USER_MODULE_NAME*` where demos expect them).
- **Interrupts:** `isr_install` then **`sti`**; keyboard IRQ can fill the ring buffer consumed by **`SYSCALL_GETC`** in user mode.
- **User demo:** builds an address space, copies the module image, then **`process_start`** returns to user via **IRET**; user code uses **`int 0x80`** for syscalls.
- **Always ends** in **`boot_idle_forever`** so the CPU does not fall off the end of `kmain`.

---

## 3. Runtime request paths (after `sti`)

Three common ways control enters kernel code paths besides the boot sequence:

```mermaid
flowchart LR
  subgraph ring3 [Ring 3 user]
    INT80["int 0x80"]
  end
  subgraph syscall_path [Syscall path]
    ENT["syscall_entry (asm)"]
    HAND["syscall_handler (C)"]
  end
  INT80 --> ENT --> HAND

  subgraph hw [Hardware]
    IRQ["IRQ via PIC"]
  end
  subgraph irq_path [Interrupt path]
    IDT["IDT stub"]
    IH["interrupt_handler (C)"]
  end
  IRQ --> IDT --> IH

  subgraph vfs_path [VFS path]
    VFS["vfs_* APIs"]
    RES["vfs_resolve_fs: /dev/ → devfs, else ramfs"]
    FS["ramfs / devfs ops"]
  end
  VFS --> RES --> FS
```

---

## 4. One-paragraph summary

**GRUB** hands off to **`boot/loader.asm`**, which calls **`kmain`**. **`kmain`** runs staged **`boot_*`** functions: console, Multiboot check, physical memory and heap, process and syscall setup, **Multiboot module** registration, **VFS** (RAMFS + devfs), optional filesystem self-tests, **IDT + PIC + keyboard**, optional memory tests, optional **user process** from a named module, then an **idle loop** with **HLT**. At runtime, **syscalls** use **`int 0x80`**, hardware uses **IRQs** through the **IDT**, and files go through **VFS** into **RAMFS** or **devfs** depending on path prefix.
