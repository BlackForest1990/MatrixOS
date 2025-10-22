; user/programs/hello.asm
; 用户程序测试系统调用

section .text
bits 32          ; ← 添加这行！确保生成32位代码
global _start

; 系统调用号
SYS_EXIT     equ 1
SYS_PUTS     equ 2
SYS_PUTCHAR  equ 4

_start:
    ; 用户程序入口点
    
    ; 设置数据段（用户数据段选择子 0x23）
    mov ax, 0x23
    mov ds, ax
    mov es, ax
    
    ; 打印欢迎消息
    mov eax, SYS_PUTS
    mov ebx, welcome_msg
    int 0x80
    
    ; 逐个字符打印测试
    mov eax, SYS_PUTCHAR
    mov ebx, '='
    int 0x80
    mov ebx, '='
    int 0x80
    mov ebx, '>'
    int 0x80
    mov ebx, ' '
    int 0x80
    mov ebx, 'U'
    int 0x80
    mov ebx, 's'
    int 0x80
    mov ebx, 'e'
    int 0x80
    mov ebx, 'r'
    int 0x80
    mov ebx, ' '
    int 0x80
    mov ebx, 'M'
    int 0x80
    mov ebx, 'o'
    int 0x80
    mov ebx, 'd'
    int 0x80
    mov ebx, 'e'
    int 0x80
    mov ebx, 0x0A   ; 换行
    int 0x80
    
    ; 打印系统信息
    mov eax, SYS_PUTS
    mov ebx, info_msg
    int 0x80
    
    ; 退出程序
    mov eax, SYS_EXIT
    int 0x80
    
    ; 如果系统调用失败，进入无限循环
.hang:
    jmp .hang

section .data
align 4
welcome_msg db "MatrixOS User Program Started", 0
info_msg    db "System calls working correctly!", 0