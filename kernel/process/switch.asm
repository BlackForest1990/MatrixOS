; kernel/process/switch.asm
; 用户模式切换汇编函数

section .text

; 用户模式段选择子
USER_CODE_SELECTOR equ 0x1B
USER_DATA_SELECTOR equ 0x23

; void switch_to_user_mode(uint32_t page_dir_phys, uint32_t entry_point, uint32_t stack_top)
global switch_to_user_mode
switch_to_user_mode:
    ; 参数:
    ;   [esp+4] = 用户页目录物理地址
    ;   [esp+8] = 用户程序入口点  
    ;   [esp+12] = 用户栈顶地址

    cli
    mov eax, [esp+4]           ; 页目录物理地址
    mov ebx, [esp+8]           ; 入口点
    mov ecx, [esp+12]          ; 栈顶地址

    ; 切换到用户页目录
    mov cr3, eax

    ; 设置用户数据段寄存器
    mov ax, USER_DATA_SELECTOR
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; 在栈上构造IRET帧
    push USER_DATA_SELECTOR     ; SS
    push ecx                    ; ESP
    push 0x202                  ; EFLAGS (IF=1)
    push USER_CODE_SELECTOR     ; CS
    push ebx                    ; EIP

    ; 切换到用户模式
    iret

; 删除下面的 syscall_entry 相关代码