# MatrixOS — operating system development notes

Practical notes for building a small **32-bit x86** teaching kernel: enough technical structure to navigate the tree, without pasting the whole codebase. Current build and run steps are in the root **[README.md](../README.md)**.

---

## Introduction

This document complements the repository: it explains how the major pieces fit together (memory, processes, interrupts, VFS, syscalls). The kernel is MIT-licensed; see [`LICENSE`](../LICENSE).

The author’s goal is a **lightweight, runnable** system that exercises process management, memory management, file-system plumbing, and I/O — in the spirit of studying how real OSes layer these concerns.

---

## Environment

- Developed and exercised on **GNU** toolchains (example host: **Ubuntu 24.04 LTS** in the original notes).
- Debug is typically **QEMU + GDB**; any equivalent setup is fine.

---

## Architecture overview

### Layered design

1. **HAL** — interrupts, memory, device drivers  
2. **Kernel core** — scheduler/PCB, filesystem, system calls  
3. **User space** — flat binaries loaded as GRUB modules  

---

## Core subsystems (map to sources)

### 1. Memory management

- **Buddy allocator** (`pmm.c`): power-of-two blocks up to **8MB** (`MAX_ORDER`), bitmap + free lists.  
- **Kernel heap** (`kmalloc.c`): first/best-fit style list, `kmalloc` / `kfree` / `kcalloc` / `krealloc`, split/merge.  
- **Virtual memory** (`vmm.c`): per-user page directory, load user binary, map code at **`0x00000000`**, user stack page at **`0xBFFFF000`** (top **`0xBFFFFFFB`**).  
- **Temp mapping** (`temp_mapping.c`): a few fixed high virtual slots to touch arbitrary physical pages safely during bring-up.

### 2. Processes

- **PCB** (`process.h`): PID, state, name, page directory, entry, user/kernel stacks.  
- **Ring switch** (`switch.asm`, IRET path): user **CS=0x1B**, **DS/SS=0x23**, full register save/restore.

### 3. Filesystem

- **VFS** (`vfs.c`): unified `open/read/write/close/seek/ioctl/stat`, fd table (**32** entries), stdin/stdout/stderr (**0/1/2**).  
- **RAMFS** (`ramfs.c`): GRUB modules exposed as **read-only** in-memory files (`0444`), up to **64** files.  
- **DEVFS** (`devfs.c`): **`/dev/console`**, **`/dev/null`**, **`/dev/zero`**.

### 4. Interrupts and syscalls

- **IDT**: 256 vectors; exceptions **0–31**; hardware IRQs after PIC remap (**32–47**); **`int 0x80`** trap gate (**DPL=3**) for syscalls.  
- **PIC**: remapped to **0x20–0x2F**; **EOI** to master/slave as needed.  
- **Keyboard** (`keyboard.c`): PS/2 scan codes → ring buffer → **`SYSCALL_GETC`**.

### 5. Drivers (short)

- **Framebuffer** (`fb.c`): VGA text **80×25** at **`0xC00B8000`**, attribute bytes, cursor via **0x3D4/0x3D5**, scroll/clear.  
- **Serial** (`serial.c`): **COM1 0x3F8**, **115200**, `serial_printf` / log levels.  
- **Keyboard**: IRQ **1** → handler pushes ASCII into **`kbuf`**.

---

## Cross-cutting themes

### Memory pipeline (conceptual)

```c
// Physical pages → buddy; small kernel objects → kmalloc;
// per-process layout → VMM; one-off physical peek → temp_map_page.
```

### Process isolation

- Separate page directories; ring **3** user / ring **0** kernel; **syscalls** as controlled entry.

### Filesystem stack (conceptual)

```text
User → syscall → VFS → RAMFS or DEVFS → memory or hardware
```

### Errors

- Syscalls and VFS return **negative** codes (`ENOENT`, `EACCES`, `EMFILE`, `EINVAL`, … — see `kernel/fs/vfs.h`).

---

## User programs

### Syscall pattern (NASM)

```nasm
mov eax, SYS_PUTS
mov ebx, message
int 0x80
```

### Loading

- GRUB **Multiboot modules** → RAMFS file names → loader/`process_create_from_module` (see **`include/boot_config.h`** for demo module names).

---

## Framebuffer driver (`fb.c`)

- **VGA text buffer** mapped at **`0xC00B8000`**: cells are **[char][attr]** (16 foreground/background colors).  
- Core APIs: `fb_write`, `fb_putchar`, `fb_puts`, cursor get/set/move, `fb_clear` / `fb_scroll`, `fb_set_color`.  
- Special characters: **`\n`**, **`\t`** (teaching: e.g. 4 spaces), **`\b`**, **`\r`** handled in the driver loop.

---

## Serial driver (`serial.c`)

- **COM1**, divisor for baud, **8N1**, FIFO enabled where configured.  
- `serial_init`, `serial_write_char`, `serial_write`, `serial_printf` / `serial_vprintf`, optional `serial_log` with levels.  
- Typical `serial_write_char`: spin on **THR empty**, then `outb`.

---

## Segmentation (GDT)

Teaching layout (flat segments, privilege via **DPL**):

- **0x08** kernel code, **0x10** kernel data  
- **0x18** user code, **0x20** user data (**DPL=3**)  
- **TSS** descriptor for privilege stack on ring transitions  
- **Syscall path** uses **`int 0x80`** with a user-callable gate — paging carries most of the real protection story.

---

## Interrupt handling

### IDT entry (shape)

```c
struct idt_entry {
    uint16_t base_low;
    uint16_t sel;       /* e.g. KERNEL_CS 0x08 */
    uint8_t  zero;
    uint8_t  flags;    /* trap/interrupt gate + DPL */
    uint16_t base_high;
};
```

### Stub pattern (`interrupt_asm.asm`)

- Macros **`ISR_NOERR` / `ISR_ERR`** push vector (and dummy error code if needed) → **`isr_common_stub`**: **PUSHA**, kernel **DS**, **`interrupt_handler(regs, int_no)`**, restore, **EOI**, **IRET**.

### PIC

- `pic_remap` sets master/slave offsets (**0x20** / **0x28**); **`pic_eoi`** for IRQ **≥8** hits slave then master.

---

## Keyboard input

- Ring buffer **`kbuf`** (**256** bytes), head/tail indices.  
- `keyboard_init` sequence (controller enable/disable as in source), **`keyboard_input_handler`** filters ack bytes, maps scan codes, **`keyboard_get_char`** drains buffer.  
- Wired to **IRQ1** and **`SYSCALL_GETC`** in `syscall.c`.

---

## Memory management (detail)

### Virtual layout (teaching map)

| Virtual range | Use | Backing |
|----------------|-----|---------|
| `0x00000000`–`0x003FFFFF` | User code (~4MB window) | Per-process tables |
| `0xBFFFF000`–`0xBFFFFFFF` | User stack page | Per-process |
| `0xC0000000`–… | Kernel text/data | Linked kernel mapping |
| `0xC00B8000`–… | VGA | Direct / mapped window |
| `0xC03FC000`–`0xC03FFFFF` | Temp-map slots | Ephemeral |
| `0xFFC00000`–`0xFFFFFFFF` | Recursive page table view | Self-map trick |

### Bring-up ideas in tree

- **Identity map** low memory during early boot; **higher-half** link at **`0xC0000000`**.  
- **Self-map** last PDE so **`0xFFC00000`** indexes page tables (handy for debug).  
- **`vmm_create_user_space`**: clone kernel PDE entries (**768–1023**) so user processes always see the kernel.  
- **`vmm_load_user_program`**: map code + stack PTEs, **`memcpy`** via **`temp_map_page`** from module physical memory.  
- **Buddy** `pmm_alloc_pages` / `pmm_free_pages` with split/coalesce; **kmalloc** header list with coalesce on `kfree`.

### Typical flows

**New user process:** `vmm_create_user_space` → allocate code/stack pages → copy image → install PCB → later **`switch_to_user_mode`**.  
**Kernel allocation:** small objects **`kmalloc`**, whole pages **`pmm_alloc_pages`**, temporary physical peek **`temp_map_page`**.

---

## User mode

- **Privilege**: Ring **0** kernel vs Ring **3** user; hardware enforces descriptor limits with flat segments.  
- **IRET frame**: **SS**/**ESP**/**EFLAGS**/**CS**/**EIP** pushed so **`iret`** lands in user ring with interrupts on (**IF** as configured in the frame).  
- **`process_create_from_module`**: locate module → `vmm_create_user_space` → `vmm_load_user_program` → kernel stack page for syscall/interrupt paths → fill PCB (**entry 0**, **user stack top** macro).

---

## Filesystem architecture

### VFS

- **`struct file_operations`**: pointers for **open/close/read/write/seek/ioctl/stat**.  
- **`struct file_handle`**: fs pointer, per-open **`private_data`**, position, flags, **`ref_count`**.  
- **`vfs_resolve_fs`**: teaching rule — paths under **`/dev/`** → **devfs**, else **ramfs** (see comments in `vfs.c` for future mount tables).

### RAMFS

- **`ramfs_init`**: walk Multiboot modules, register each as a file (name/size/readonly).  
- **`ramfs_open`**: find file, permission check vs **`O_*`**, allocate per-fd private state.  
- **`ramfs_read`**: bounds-checked **`memcpy`** from module memory.

### DEVFS

- Resolve **`/dev/console`**, **`/dev/null`**, **`/dev/zero`**.  
- **Write** console → **`fb_putchar`** loop; **read** console → **`keyboard_get_char`**; **ioctl** teaching hooks (e.g. clear screen / query cursor — match **`syscall.c`** / **`devfs.c`** in tree).

### Integration tests

- See **`kernel/fs/test_fs.c`** and **`include/boot_config.h`** for module names used in self-tests.

---

## System calls

### Chain

```text
User (ring 3) → INT 0x80 → syscall_entry (asm) → syscall_handler (C) → IRET back
```

### Gate

- Vector **0x80** installed with **DPL=3** so user mode may invoke it; ordinary IRQ gates stay **DPL=0**.

### Calling convention

- **`EAX`**: number; **`EBX`/`ECX`/`EDX`**: args; **`EAX`**: return.  
- Full table: **[syscall-table.md](syscall-table.md)** (keep in sync with **`user/programs/*.asm`** `equ` values).

### Assembly entry (concept)

- Save segment + GPRs, kernel **DS**, call **`syscall_handler(struct regs *r)`**, restore, **`iret`**.

---

## Closing

These notes track the **teaching** kernel in this repository. When in doubt, prefer reading the cited **`.c` / `.asm` / `.h`** files and the root **README** for exact build flags and demo switches.
