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
    dq 0

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

section .text:                  ; start of the text (code) section
global loader
extern kmain

loader:                         ; the loader label (defined as entry point in linker script)

        ; --- 新增：加载 GDT ---
        cli                     ; 可选：禁用中断
        lgdt [gdt_descriptor]   ; 加载 GDT

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

        ; --- 调用主内核函数 ---
        call kmain

.loop:
        jmp .loop               ; loop forever

section .bss 
align 4                         ; align at 4 bytes
kernel_stack:                   ; label points to beginning of memory
    resb KERNEL_STACK_SIZE      ; reserve stack for the kernel
