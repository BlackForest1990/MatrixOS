# 工具配置
CC = gcc
AS = nasm 
LD = ld

# 编译标志

CFLAGS = -m32 \
         -nostdlib \
         -nostdinc \
         -fno-builtin \
         -fno-stack-protector \
         -nostartfiles \
         -nodefaultlibs \
         -Wall -Wextra -Werror \
         -c \
         -O0 \
	 -g \
         -Iinclude -Ikernel -Ikernel/drivers -Ikernel/interrupts -Ikernel/mm -Ilib \
         -fno-pic \
         -fno-pie \
         -static \
         -fcf-protection=none

LDFLAGS = -T link.ld -melf_i386
ASFLAGS = -f elf32 -g

# C源文件列表
KERNEL_C_SRCS = kernel/kmain.c \
                kernel/drivers/fb.c \
                kernel/drivers/serial.c \
                kernel/interrupts/idt.c \
                kernel/interrupts/interrupt.c \
                kernel/interrupts/pic.c \
                kernel/interrupts/keyboard.c \
		kernel/mm/kmalloc.c \
		kernel/mm/pmm.c \
		kernel/mm/temp_mapping.c \
		kernel/mm/test_mm.c \
		lib/string.c

# 汇编源文件列表
KERNEL_ASM_SRCS = kernel/drivers/io.asm \
                  kernel/interrupts/interrupt_asm.asm

# 引导加载程序汇编文件
BOOT_SRC = boot/loader.asm

# 对象文件列表
BOOT_OBJ = $(BOOT_SRC:.asm=.o)
KERNEL_C_OBJS = $(KERNEL_C_SRCS:.c=.o)
KERNEL_ASM_OBJS = $(KERNEL_ASM_SRCS:.asm=.o)
OBJECTS = $(BOOT_OBJ) $(KERNEL_C_OBJS) $(KERNEL_ASM_OBJS)

all: kernel.elf

kernel.elf: $(OBJECTS)
	ld $(LDFLAGS) $(OBJECTS) -o kernel.elf

os.iso: kernel.elf
	mkdir -p iso/boot/grub
	cp kernel.elf iso/boot/kernel.elf
	cp boot/grub/grub.cfg iso/boot/grub/
	grub-mkrescue -o os.iso iso   

# 快速运行 QEMU
qemu: kernel.elf
	qemu-system-i386 -kernel kernel.elf -serial stdio -d int -no-reboot -no-shutdown

# 新增：启动 QEMU 并等待 GDB 连接
debug-qemu: kernel.elf
	qemu-system-i386 -kernel kernel.elf \
		-s -S \
		-serial stdio \
		-d int \
		-D qemu.log \
		-no-reboot \
		-no-shutdown
		# ^^^^ -s: 启用 GDB server (port 1234), -S: 启动时暂停 CPU

# 运行bochs
run: os.iso
	bochs -f bochsrc.txt -q

# 编译规则
%.o: %.c
	$(CC) $(CFLAGS) $< -o $@

%.o: %.asm
	$(AS) $(ASFLAGS) $< -o $@

#清理
clean:
	find . -name "*.o" -delete
	rm -f kernel.elf os.iso bochslog.txt com1.out
	rm -rf iso/

# 删除中间文件时出错则中断
.DELETE_ON_ERROR:
