MAGIC_NUMBER equ  0x1BADB002    ; define the magic number constant
FLAGS        equ  0x0		; multiboot flags
CHECKSUM     equ  -MAGIC_NUMBER ; calculate the checksum
                                ; (magic number + checksum + flags should equal 0)
KERNEL_STACK_SIZE equ 4096      ; size of stack in bytes

section .multiboot
align 4                         ; the code must be 4 byte aligned
         dd MAGIC_NUMBER         ; write the magic number to the machine code,
         dd FLAGS                ; the flags,
         dd CHECKSUM             ; and the checksum

; --- 新增：GDT 定义 ---
section .data
align 4
; GDT 表开始
gdt_start:
    ; 空描述符 (索引 0)
    dq 0x0000000000000000  ; 空描述符

; 代码段描述符 (索引 1, 选择子 = 0x08)
gdt_code:
    dw 0xffff       ; 段界限 (0-15)
    dw 0x0000       ; 段基址 (0-15)
    db 0x00         ; 段基址 (16-23)
    db 10011010b    ; 类型标志: P=1, DPL=00, S=1, Type=1010 (执行/读)
    db 11001111b    ; 其他标志: G=1, D/B=1, L=0, AVL=0, 段界限 (16-19)=1111
    db 0x00         ; 段基址 (24-31)

; 数据段描述符 (索引 2, 选择子 = 0x10)
gdt_data:
    dw 0xffff       ; 段界限 (0-15)
    dw 0x0000       ; 段基址 (0-15)
    db 0x00         ; 段基址 (16-23)
    db 10010010b    ; 类型标志: P=1, DPL=00, S=1, Type=0010 (读/写)
    db 11001111b    ; 其他标志: G=1, D/B=1, L=0, AVL=0, 段界限 (16-19)=1111
    db 0x00         ; 段基址 (24-31)
gdt_end:

; GDT 描述符（用于 lgdt 指令）
gdt_descriptor:
    dw gdt_end - gdt_start - 1  ; GDT 大小
    dd gdt_start                ; GDT 线性起始地址

; ----------------------------
;  分页结构（恒等映射前 4MB）
; ----------------------------

align 4096
page_directory:
    resd 1024                   ; 1024 个条目，共 4KB

align 4096
page_table_identity:
    resd 1024                   ; 用于映射 0x0 ~ 0x3FFFFF (4MB)

section .text                  ; start of the text (code) section
global loader
extern kmain

loader:                         ; the loader label (defined as entry point in linker script)

    ; --- 新增：加载 GDT ---
    cli                     ; 可选：禁用中断
    lgdt [gdt_descriptor]   ; 加载 GDT

    ; --- 新增：启用保护模式 ---
    mov eax, cr0            ; 2. 读取CR0寄存器到EAX
    or eax, 0x1             ; 设置PE（Protection Enable）位为1
    mov cr0, eax            ; 3. 写回CR0，正式启用保护模式！

    ; --- 设置代码段 (CS) ---
    jmp 0x08:flush_cs       ; 远跳转：使用选择子 0x08 (代码段) 更新 CS
flush_cs:

    ; --- 设置数据段寄存器 ---
    mov ax, 0x10            ; 选择子 0x10 (数据段)
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax              ; 栈段也使用数据段

    ; --- 设置栈指针 ---
    mov esp, kernel_stack + KERNEL_STACK_SIZE

    ; ============================
    ;  恒等映射：0x0 -> 0x0 (4MB)
    ; ============================

    ; 1. 清空页目录和页表（可选，但推荐）
    mov edi, page_directory
    mov ecx, 1024
    xor eax, eax
    rep stosd                   ; 清空页目录

    mov edi, page_table_identity
    mov ecx, 1024
    xor eax, eax
    rep stosd                   ; 清空页表

    ; 2. 填充页表：映射 0x00000000 ~ 0x003FFFFF
    mov edi, page_table_identity
    mov ebx, 0x00000000         ; 当前物理地址
    mov ecx, 1024               ; 1024 个页 (4MB / 4KB)

.fill_identity:
    mov eax, ebx
    or eax, 0x00000003          ; Present + Writable
    mov [edi], eax
    add edi, 4
    add ebx, 0x1000             ; 下一页
    loop .fill_identity

    ; 3. 将页表映射到页目录第 0 项
    mov eax, page_table_identity
    or eax, 0x00000003          ; Present + Writable
    mov [page_directory], eax   ; 页目录[0] = 页表地址

    ; 4. （可选）将页目录映射到自身（用于以后动态管理页表）
    ; mov [page_directory + 0x3FC*4], eax ; 0xFFC00000 -> page_directory
    ; 我们暂时不需要，跳过

    ; 5. 设置 cr3 指向页目录
    mov eax, page_directory
    mov cr3, eax

    ; 6. 启用 PSE（为未来使用 4MB 页做准备）
    mov eax, cr4
    or eax, 0x00000010          ; 设置 PSE bit (bit 4)
    mov cr4, eax

    ; 7. 启用分页！
    mov eax, cr0
    or eax, 0x80000000          ; 设置 PG bit
    mov cr0, eax

    ; ✅ 分页已启用！
    ; 现在所有地址访问都会经过页表转换
    ; 虚拟地址 0x0 ~ 0x3FFFFF 已映射到物理地址 0x0 ~ 0x3FFFFF

    ; --- 调用主内核函数 ---
    call kmain

.loop:
    jmp .loop               ; loop forever

section .bss 
align 4                         ; align at 4 bytes
kernel_stack:                   ; label points to beginning of memory
    resb KERNEL_STACK_SIZE      ; reserve stack for the kernel

