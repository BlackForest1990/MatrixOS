; kernel/syscall/syscall_asm.asm
; 系统调用汇编入口

section .text

; 系统调用入口点 (INT 0x80)
global syscall_entry
extern syscall_handler

syscall_entry:
    ; 保存所有寄存器
    pusha
    push ds
    push es
    push fs
    push gs
    
    ; 设置内核数据段
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    ; 传递寄存器结构指针（栈顶就是结构指针）
    push esp
    call syscall_handler
    add esp, 4
    
    ; 恢复环境
    pop gs
    pop fs
    pop es
    pop ds
    popa
    
    iret

