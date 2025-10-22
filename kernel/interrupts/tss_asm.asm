; kernel/interrupts/tss_asm.asm
; TSS 加载函数

section .text

global tss_flush
global tss_set_descriptor
extern gdt_tss      ; 声明外部符号


; void tss_flush(void)
tss_flush:
    mov ax, 0x28      ; TSS 段选择子 (索引 5, TI=0, RPL=00)
    ltr ax            ; 加载任务寄存器
    ret

; void tss_set_descriptor(uint32_t tss_base)
tss_set_descriptor:
    mov eax, [esp + 4]    ; 获取 tss_base 参数
    
    ; 设置 GDT 中 TSS 描述符的基地址
    mov [gdt_tss + 2], ax      ; Base 0-15
    shr eax, 16
    mov [gdt_tss + 4], al      ; Base 16-23  
    mov [gdt_tss + 7], ah      ; Base 24-31
    ret