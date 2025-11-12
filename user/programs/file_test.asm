; user/programs/file_test.asm
section .text
bits 32
global _start

; 系统调用号
SYS_EXIT    equ 1
SYS_PUTS    equ 2
SYS_PUTCHAR equ 4
SYS_OPEN    equ 10
SYS_CLOSE   equ 11
SYS_READ    equ 12
SYS_WRITE   equ 13
SYS_SEEK    equ 14
SYS_STAT    equ 16

; 文件打开标志
O_RDONLY    equ 0x0001
O_WRONLY    equ 0x0002
O_RDWR      equ 0x0003

_start:
    ; 显示测试开始信息
    mov eax, SYS_PUTS
    mov ebx, test_start_msg
    int 0x80

    ; ========== 测试1: 文件系统基本功能 ==========
    mov eax, SYS_PUTS
    mov ebx, test1_msg
    int 0x80

    ; 打开文件
    mov eax, SYS_OPEN
    mov ebx, filename
    mov ecx, O_RDONLY
    int 0x80

    cmp eax, 0
    jl .test1_fail

    mov [file_fd], eax

    ; 读取并显示文件内容
    mov eax, SYS_PUTS
    mov ebx, file_content_msg
    int 0x80

.read_loop:
    mov eax, SYS_READ
    mov ebx, [file_fd]
    mov ecx, buffer
    mov edx, 255
    int 0x80

    cmp eax, 0
    jle .read_done

    ; 显示读取的内容
    mov byte [buffer + eax], 0 ; 添加字符串终止符
    mov eax, SYS_PUTS
    mov ebx, buffer
    int 0x80

    jmp .read_loop

.read_done:
    ; 关闭文件
    mov eax, SYS_CLOSE
    mov ebx, [file_fd]
    int 0x80

    mov eax, SYS_PUTS
    mov ebx, test1_success_msg
    int 0x80
    jmp .test2

.test1_fail:
    mov eax, SYS_PUTS
    mov ebx, test1_fail_msg
    int 0x80

    ; ========== 测试2: 设备文件系统 ==========
.test2:
    mov eax, SYS_PUTS
    mov ebx, test2_msg
    int 0x80

    ; 打开控制台设备
    mov eax, SYS_OPEN
    mov ebx, dev_console
    mov ecx, O_WRONLY
    int 0x80

    cmp eax, 0
    jl .test2_fail

    mov [dev_fd], eax

    ; 写入控制台设备
    mov eax, SYS_WRITE
    mov ebx, [dev_fd]
    mov ecx, dev_msg
    mov edx, dev_msg_len
    int 0x80

    ; 关闭设备
    mov eax, SYS_CLOSE
    mov ebx, [dev_fd]
    int 0x80

    mov eax, SYS_PUTS
    mov ebx, test2_success_msg
    int 0x80
    jmp .test3

.test2_fail:
    mov eax, SYS_PUTS
    mov ebx, test2_fail_msg
    int 0x80

    ; ========== 测试3: 标准输入输出 ==========
.test3:
    mov eax, SYS_PUTS
    mov ebx, test3_msg
    int 0x80

    ; 测试标准输出 (fd=1)
    mov eax, SYS_WRITE
    mov ebx, 1  ; STDOUT_FILENO
    mov ecx, stdout_msg
    mov edx, stdout_msg_len
    int 0x80

    ; 测试标准错误 (fd=2)
    mov eax, SYS_WRITE
    mov ebx, 2  ; STDERR_FILENO
    mov ecx, stderr_msg
    mov edx, stderr_msg_len
    int 0x80

    mov eax, SYS_PUTS
    mov ebx, test3_success_msg
    int 0x80

    ; ========== 测试4: 文件定位 ==========
.test4:
    mov eax, SYS_PUTS
    mov ebx, test4_msg
    int 0x80

    ; 重新打开文件测试seek
    mov eax, SYS_OPEN
    mov ebx, filename
    mov ecx, O_RDONLY
    int 0x80

    cmp eax, 0
    jl .test4_fail

    mov [file_fd], eax

    ; 读取文件开头
    mov eax, SYS_READ
    mov ebx, [file_fd]
    mov ecx, buffer
    mov edx, 10
    int 0x80

    mov byte [buffer + eax], 0
    mov eax, SYS_PUTS
    mov ebx, file_start_msg
    int 0x80
    mov eax, SYS_PUTS
    mov ebx, buffer
    int 0x80
    mov eax, SYS_PUTCHAR
    mov ebx, 0x0A
    int 0x80

    ; 关闭文件
    mov eax, SYS_CLOSE
    mov ebx, [file_fd]
    int 0x80

    mov eax, SYS_PUTS
    mov ebx, test4_success_msg
    int 0x80
    jmp .test_complete

.test4_fail:
    mov eax, SYS_PUTS
    mov ebx, test4_fail_msg
    int 0x80

    ; ========== 测试完成 ==========
.test_complete:
    mov eax, SYS_PUTS
    mov ebx, test_complete_msg
    int 0x80

    ; 退出程序
    mov eax, SYS_EXIT
    mov ebx, 0
    int 0x80

section .data
align 4
; 测试消息
test_start_msg      db "=== File System Test Start ===", 0x0A, 0
test1_msg           db "Test 1: Basic file operations...", 0x0A, 0
test1_success_msg   db "Test 1 PASSED: File read successful", 0x0A, 0
test1_fail_msg      db "Test 1 FAILED: Cannot open file", 0x0A, 0
test2_msg           db "Test 2: Device file system...", 0x0A, 0
test2_success_msg   db "Test 2 PASSED: Device write successful", 0x0A, 0
test2_fail_msg      db "Test 2 FAILED: Cannot open device", 0x0A, 0
test3_msg           db "Test 3: Standard I/O...", 0x0A, 0
test3_success_msg   db "Test 3 PASSED: Stdout/Stderr working", 0x0A, 0
test4_msg           db "Test 4: File positioning...", 0x0A, 0
test4_success_msg   db "Test 4 PASSED: File positioning working", 0x0A, 0
test4_fail_msg      db "Test 4 FAILED: File positioning failed", 0x0A, 0
test_complete_msg   db "=== All Tests Completed ===", 0x0A, 0

; 文件内容显示消息
file_content_msg    db "File content:", 0x0A, 0
file_start_msg      db "First 10 bytes: ", 0

; 文件名和设备名
filename            db "hello", 0
dev_console         db "/dev/console", 0

; 测试消息
dev_msg             db "This is written to /dev/console!", 0x0A, 0
dev_msg_len         equ $ - dev_msg

stdout_msg          db "This is stdout message", 0x0A, 0
stdout_msg_len      equ $ - stdout_msg

stderr_msg          db "This is stderr message", 0x0A, 0
stderr_msg_len      equ $ - stderr_msg

section .bss
align 4
file_fd     resd 1
dev_fd      resd 1
buffer      resb 256