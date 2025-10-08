; ============================================
;  loader.asm - 高半地址内核加载器（修复版）
; ============================================

%define MAGIC_NUMBER    0x1BADB002
%define FLAGS           0x0
%define CHECKSUM        (-(MAGIC_NUMBER + FLAGS))
%define KERNEL_STACK_SIZE 4096

; 虚拟地址偏移
%define VIRT_OFFSET     0xC0000000

; 物理地址基址
%define PHYS_BASE       0x100000

; 虚拟地址基址
%define VIRT_BASE       (PHYS_BASE + VIRT_OFFSET)

; 页大小
%define PAGE_SIZE       4096

SECTION .multiboot.data
    align 4
    dd MAGIC_NUMBER
    dd FLAGS
    dd CHECKSUM

SECTION .multiboot.text
global loader
extern kmain
extern kernel_virtual_start
extern kernel_physical_start

loader:
    cli                         ; 关中断
    mov esp, temporary_stack + 4096 - VIRT_OFFSET  ; 使用临时栈（物理地址）

    ; === 1. 清空页目录和页表 ===
    mov edi, page_directory - VIRT_OFFSET
    xor eax, eax
    mov ecx, 1024
    rep stosd

    mov edi, first_page_table - VIRT_OFFSET
    mov ecx, 1024
    rep stosd

    ; === 2. 建立 identity 映射 (0x0 ~ 4MB) ===
    ; 页目录[0] 指向 first_page_table (identity mapping)
    mov eax, first_page_table - VIRT_OFFSET
    or eax, 0x03                ; Present + Writable
    mov [page_directory - VIRT_OFFSET + 0*4], eax

    ; 填充页表：物理地址 0x0 到 0x3FFFFF (4MB)
    mov edi, first_page_table - VIRT_OFFSET
    mov ebx, 0x00000000         ; 起始物理地址
    mov ecx, 1024               ; 1024 个页
.fill_identity:
    mov eax, ebx
    or eax, 0x03                ; Present + Writable
    mov [edi], eax
    add edi, 4
    add ebx, 0x1000
    loop .fill_identity

    ; === 3. 映射高半地址 (0xC0000000 ~ 0xC03FFFFF) 到物理地址 0x0 ~ 0x3FFFFF ===
    ; 页目录[768] 也指向同一个 first_page_table
    mov eax, first_page_table - VIRT_OFFSET
    or eax, 0x03
    mov [page_directory - VIRT_OFFSET + 768*4], eax

    ; === 4. 设置 CR3（页目录物理地址）===
    mov eax, page_directory - VIRT_OFFSET
    mov cr3, eax

    ; === 5. 加载 GDT（使用物理地址）===
    lgdt [gdt_descriptor - VIRT_OFFSET]

    ; === 6. 启用保护模式 ===
    mov eax, cr0
    or eax, 0x1
    mov cr0, eax

    ; === 7. 启用分页 ===
    mov eax, cr0
    or eax, 0x80000000
    mov cr0, eax

    ; === 从此刻起，所有地址都是虚拟地址！===

    ; === 8. 跳转到高半地址 ===
    ; 使用相对跳转而不是硬编码地址
    lea eax, [high_half_entry]
    jmp eax

; ============================================
;  高半部入口（在 .text 段，链接到高半地址）
; ============================================
SECTION .text
global high_half_entry
high_half_entry:
    ; 重新加载 GDT（现在使用虚拟地址）
    lgdt [gdt_descriptor]

    ; 刷新段寄存器
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; 设置高半地址栈
    mov esp, kernel_stack_top

    ; 调用内核主函数
    call kmain

.hang:
    hlt
    jmp .hang

; ============================================
;  数据段
; ============================================
SECTION .data

; --- GDT ---
align 4
gdt_start:
    dq 0x0000000000000000      ; 空描述符
gdt_code:
    dw 0xFFFF                 ; Limit 0-15
    dw 0x0000                 ; Base 0-15  
    db 0x00                   ; Base 16-23
    db 10011010b              ; P=1, DPL=0, S=1, Type=1010 (代码段，可读，非一致)
    db 11001111b              ; G=1, D/B=1, L=0, Limit 16-19=1111
    db 0x00                   ; Base 24-31
gdt_data:
    dw 0xFFFF                 ; Limit 0-15
    dw 0x0000                 ; Base 0-15
    db 0x00                   ; Base 16-23
    db 10010010b              ; P=1, DPL=0, S=1, Type=0010 (数据段，可写，向上扩展)
    db 11001111b              ; G=1, D/B=1, L=0, Limit 16-19=1111
    db 0x00                   ; Base 24-31
gdt_end:

; GDT 描述符  
gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

; ============================================
;  BSS 段（未初始化数据）
; ============================================
SECTION .bss
align 4096
page_directory:
    resb 4096

align 4096  
first_page_table:
    resb 4096

; 临时栈（分页前使用）
align 4
temporary_stack:
    resb 4096

; 内核栈（高半地址使用）
align 4
global kernel_stack_bottom
kernel_stack_bottom:
    resb KERNEL_STACK_SIZE
global kernel_stack_top
kernel_stack_top:

