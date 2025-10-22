; ============================================
;  loader.asm - 支持 multiboot 模块的加载器
; ============================================

%define MAGIC_NUMBER    0x1BADB002
%define FLAGS           0x00000003
%define CHECKSUM        (-(MAGIC_NUMBER + FLAGS))
%define KERNEL_STACK_SIZE 16384

; 虚拟地址偏移
%define VIRT_OFFSET     0xC0000000
%define PHYS_BASE       0x100000
%define PAGE_SIZE       4096
%define MAPPED_MEMORY_SIZE (128 * 1024 * 1024)
%define PAGE_TABLE_COUNT (MAPPED_MEMORY_SIZE / (1024 * PAGE_SIZE))

SECTION .multiboot.data
align 4
    dd MAGIC_NUMBER
    dd FLAGS
    dd CHECKSUM
    dd 0, 0, 0, 0, 0, 0, 0, 0

SECTION .multiboot.text
global loader
extern kmain

loader:
    cli
    mov esp, temporary_stack + 4096 - VIRT_OFFSET

    ; 保存 multiboot 信息
    mov [multiboot_magic - VIRT_OFFSET], eax
    mov [multiboot_info - VIRT_OFFSET], ebx

    ; 清空页目录和页表
    mov edi, page_directory - VIRT_OFFSET
    xor eax, eax
    mov ecx, 1024
    rep stosd

    mov edi, first_page_table - VIRT_OFFSET
    mov ecx, 1024 * PAGE_TABLE_COUNT
    rep stosd

    ; 建立 identity 映射
    mov edi, page_directory - VIRT_OFFSET
    mov esi, first_page_table - VIRT_OFFSET
    mov ecx, PAGE_TABLE_COUNT
    mov ebx, 0
.setup_identity_pagedir:
    mov eax, esi
    or eax, 0x03
    mov [edi + ebx*4], eax
    add esi, 4096
    inc ebx
    loop .setup_identity_pagedir

    ; 填充 identity 页表
    mov edi, first_page_table - VIRT_OFFSET
    mov ebx, 0x00000000
    mov ecx, 1024 * PAGE_TABLE_COUNT
.fill_identity:
    mov eax, ebx
    or eax, 0x03
    mov [edi], eax
    add edi, 4
    add ebx, 0x1000
    loop .fill_identity

    ; 映射高半地址
    mov edi, page_directory - VIRT_OFFSET
    mov esi, first_page_table - VIRT_OFFSET
    mov ecx, PAGE_TABLE_COUNT
    mov ebx, 768
.setup_high_half_pagedir:
    mov eax, esi
    or eax, 0x03
    mov [edi + ebx*4], eax
    add esi, 4096
    inc ebx
    loop .setup_high_half_pagedir

    ; 页目录自映射
    mov eax, page_directory - VIRT_OFFSET
    or eax, 0x03
    mov [page_directory - VIRT_OFFSET + 1023*4], eax

    ; 临时映射区域
    mov eax, first_page_table - VIRT_OFFSET
    add eax, 4096 * 32
    or eax, 0x03
    mov [page_directory - VIRT_OFFSET + 769*4], eax

    mov edi, first_page_table - VIRT_OFFSET
    add edi, 4096 * 32
    mov ebx, 0x00000000
    mov ecx, 4
.fill_temp_mapping:
    mov eax, ebx
    or eax, 0x03
    mov [edi], eax
    add edi, 4
    add ebx, 0x1000
    loop .fill_temp_mapping

    ; 设置 CR3
    mov eax, page_directory - VIRT_OFFSET
    mov cr3, eax

    ; 加载 GDT
    lgdt [gdt_descriptor - VIRT_OFFSET]

    ; 启用保护模式
    mov eax, cr0
    or eax, 0x1
    mov cr0, eax

    ; 启用分页
    mov eax, cr0
    or eax, 0x80000000
    mov cr0, eax

    ; 跳转到高半地址
    lea eax, [high_half_entry]
    jmp eax

SECTION .text
global high_half_entry
high_half_entry:
    lgdt [gdt_descriptor]
    jmp 0x08:.reload_cs
.reload_cs:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, kernel_stack_top

    ; 调用内核主函数
    mov eax, [multiboot_magic]
    mov ebx, [multiboot_info]
    push ebx
    push eax
    call kmain

.hang:
    hlt
    jmp .hang

SECTION .data
align 4
gdt_start:
    ; 空描述符 (索引 0)
    dq 0x0000000000000000

    ; 内核代码段 (索引 1, 选择子 0x08)
gdt_code:
    dw 0xFFFF                 ; Limit 0-15
    dw 0x0000                 ; Base 0-15  
    db 0x00                   ; Base 16-23
    db 10011010b              ; P=1, DPL=0, S=1, Type=1010
    db 11001111b              ; G=1, D/B=1, L=0, Limit 16-19=1111
    db 0x00                   ; Base 24-31

    ; 内核数据段 (索引 2, 选择子 0x10)
gdt_data:
    dw 0xFFFF                 ; Limit 0-15
    dw 0x0000                 ; Base 0-15
    db 0x00                   ; Base 16-23
    db 10010010b              ; P=1, DPL=0, S=1, Type=0010
    db 11001111b              ; G=1, D/B=1, L=0, Limit 16-19=1111
    db 0x00                   ; Base 24-31

    ; 用户代码段 (索引 3, 选择子 0x18)
gdt_user_code:
    dw 0xFFFF                 ; Limit 0-15
    dw 0x0000                 ; Base 0-15
    db 0x00                   ; Base 16-23
    db 11111010b              ; P=1, DPL=3, S=1, Type=1010
    db 11001111b              ; G=1, D/B=1, L=0, Limit 16-19=1111
    db 0x00                   ; Base 24-31

    ; 用户数据段 (索引 4, 选择子 0x20)  
gdt_user_data:
    dw 0xFFFF                 ; Limit 0-15
    dw 0x0000                 ; Base 0-15
    db 0x00                   ; Base 16-23
    db 11110010b              ; P=1, DPL=3, S=1, Type=0010
    db 11001111b              ; G=1, D/B=1, L=0, Limit 16-19=1111
    db 0x00                   ; Base 24-31

    ; TSS 段 (索引 5, 选择子 0x28)
gdt_tss:
    dw 104                    ; Limit (TSS 大小)
    dw 0                      ; Base 0-15 (稍后设置)
    db 0                      ; Base 16-23 (稍后设置)
    db 10001001b              ; P=1, DPL=0, Type=1001 (32位TSS)
    db 00000000b              ; G=0, AVL=0, Limit 16-19=0000
    db 0                      ; Base 24-31 (稍后设置)
gdt_end:

; GDT 描述符
gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

SECTION .bss
align 4
multiboot_magic: resd 1
multiboot_info: resd 1

align 4096
page_directory: resb 4096
first_page_table: resb 4096 * 33

align 4
temporary_stack: resb 4096

global kernel_stack_bottom
kernel_stack_bottom: resb KERNEL_STACK_SIZE
global kernel_stack_top
kernel_stack_top: