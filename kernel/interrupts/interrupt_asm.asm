; kernel/interrupts/interrupt_asm.s
; 中断处理桩和中断向量表

%macro ISR_NOERR 1
    global int_%1
    int_%1:
        push %1          ; ← 先压中断号（33）
        push 0           ; 再压伪错误码（0）
        jmp isr_common_stub
%endmacro

%macro ISR_ERR 1
    global int_%1
    int_%1:
        ; 错误码已由 CPU 压入
        push %1          ; 压中断号
        jmp isr_common_stub
%endmacro

; 异常处理桩 (0-31)
ISR_NOERR 0
ISR_NOERR 1
ISR_NOERR 2
ISR_NOERR 3
ISR_NOERR 4
ISR_NOERR 5
ISR_NOERR 6
ISR_NOERR 7
ISR_NOERR 8
ISR_ERR   9
ISR_NOERR 10
ISR_NOERR 11
ISR_NOERR 12
ISR_NOERR 13
ISR_ERR   14
ISR_NOERR 15
ISR_NOERR 16
ISR_NOERR 17
ISR_NOERR 18
ISR_NOERR 19
ISR_NOERR 20
ISR_NOERR 21
ISR_NOERR 22
ISR_NOERR 23
ISR_NOERR 24
ISR_NOERR 25
ISR_NOERR 26
ISR_NOERR 27
ISR_NOERR 28
ISR_NOERR 29
ISR_NOERR 30
ISR_NOERR 31

; IRQ 处理桩 (32-47)
%assign i 32
%rep 224
    ISR_NOERR i
    %assign i i+1
%endrep

; ================================
; 定义中断向量表
; ================================
global interrupt_vectors
interrupt_vectors:
    %assign i 0
    %rep 256
        dd int_%+i          ; expands to int_0, int_1, ...
        %assign i i+1
    %endrep

    ; 剩余 208 个设为 0
    ;%rep 208
     ;   dd 0
    ;%endrep
; ================================
; 公共中断处理桩
; ================================
extern interrupt_handler

isr_common_stub:
    pusha                    ; 保存通用寄存器
    mov ax, ds
    push eax                 ; 保存原数据段

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; 此时栈布局（从低地址到高地址）：
    ;   [esp+0]:  eax_save (原 ds)
    ;   [esp+4]:  gs,fs,es,ds,edi,esi,ebp,ebx,edx,ecx,eax
    ;   [esp+36]: 0       ← 伪错误码
    ;   [esp+40]: 33      ← 中断号
    ;
    ; 我们要调用 interrupt_handler(33, &regs)

    push dword [esp + 40]  ; 第二个参数：int_no
    lea eax, [esp + 44]    ; 计算 regs 指针（跳过 int_no 和 err_code）
    push eax               ; 第一个参数：regs
    call interrupt_handler
    add esp, 8             ; 清理两个参数

    pop eax                ; 恢复原 ds
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    popa                   ; 恢复通用寄存器

    add esp, 8             ; 清除中断号和伪错误码

    ; 发送 EOI 到主 PIC
    mov al, 0x20
    out 0x20, al           ; EOI 命令

    sti
    iret

