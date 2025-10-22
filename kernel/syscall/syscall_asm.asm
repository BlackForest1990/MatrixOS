; kernel/syscall/syscall_asm.asm
; 系统调用汇编入口

section .text

; 系统调用入口点 (INT 0x80)
global syscall_entry
extern syscall_handler

syscall_entry:
    ; 保存所有通用寄存器
    pusha
    
    ; 保存段寄存器
    push ds
    push es
    push fs
    push gs
    
    ; 加载内核数据段选择子 (0x10)
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    ; 将参数压栈（按照C调用约定从右到左）
    push edx    ; arg3
    push ecx    ; arg2
    push ebx    ; arg1
    push eax    ; syscall_num (系统调用号)
    
    ; 调用C系统调用处理函数
    call syscall_handler
    
    ; 清理栈（4个参数 * 4字节 = 16字节）
    add esp, 16
    
    ; 如果需要返回值，可以在这里处理
    ; 比如将返回值保存到某个地方
    
    ; 恢复段寄存器
    pop gs
    pop fs
    pop es
    pop ds
    
    ; 恢复通用寄存器
    popa
    
    ; 从系统调用返回
    iret

; 简单的系统调用测试函数（可选）
global syscall_invoke
syscall_invoke:
    mov eax, [esp+4]    ; 系统调用号
    mov ebx, [esp+8]    ; 参数1
    mov ecx, [esp+12]   ; 参数2  
    mov edx, [esp+16]   ; 参数3
    int 0x80
    ret