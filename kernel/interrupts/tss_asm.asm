; kernel/interrupts/tss_asm.asm
; TSS 加载函数

section .text

global tss_flush

; void tss_flush(void)
tss_flush:
    mov ax, 0x28      ; TSS 段选择子 (索引 5, TI=0, RPL=00)
    ltr ax            ; 加载任务寄存器
    ret