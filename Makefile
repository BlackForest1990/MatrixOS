# 工具配置
CC = gcc
AS = nasm 
LD = ld

# 编译标志
CFLAGS = -m32 -nostdlib -nostdinc -fno-builtin -fno-stack-protector \
	-nostartfiles -nodefaultlibs -Wall -Wextra -Werror -c \
	-O0 -Iinclude -Ikernel -Ikernel/drivers -Ikernel/interrupts -Ilib

LDFLAGS = -T link.ld -melf_i386
ASFLAGS = -f elf32

# C源文件列表
KERNEL_C_SRCS = kernel/kmain.c \
                kernel/drivers/fb.c \
                kernel/drivers/serial.c \
                kernel/interrupts/idt.c \
                kernel/interrupts/interrupt.c \
                kernel/interrupts/pic.c \
                kernel/interrupts/keyboard.c

# 汇编源文件列表
KERNEL_ASM_SRCS = kernel/drivers/io.s \
                  kernel/interrupts/interrupt_asm.s

# 引导加载程序汇编文件
BOOT_SRC = boot/loader.s

# 对象文件列表
BOOT_OBJ = $(BOOT_SRC:.s=.o)
KERNEL_C_OBJS = $(KERNEL_C_SRCS:.c=.o)
KERNEL_ASM_OBJS = $(KERNEL_ASM_SRCS:.s=.o)
OBJECTS = $(BOOT_OBJ) $(KERNEL_C_OBJS) $(KERNEL_ASM_OBJS)

all: kernel.elf

kernel.elf: $(OBJECTS)
	ld $(LDFLAGS) $(OBJECTS) -o kernel.elf

os.iso: kernel.elf
	mkdir -p iso/boot/grub
	cp kernel.elf iso/boot/kernel.elf
	cp boot/grub/stage2_eltorito iso/boot/grub/
	cp boot/grub/menu.lst iso/boot/grub/
	genisoimage -R \
		-b boot/grub/stage2_eltorito \
		-no-emul-boot \
		-boot-load-size 4 \
		-A os \
		-input-charset utf8 \
		-quiet \
		-boot-info-table \
		-o os.iso \
		iso

# 快速运行 QEMU
qemu: kernel.elf
	qemu-system-i386 -kernel kernel.elf -serial stdio -d int -no-reboot -no-shutdown

# 运行bochs
run: os.iso
	bochs -f bochsrc.txt -q

# 编译规则
%.o: %.c
	$(CC) $(CFLAGS) $< -o $@

%.o: %.s
	$(AS) $(ASFLAGS) $< -o $@

#清理
clean:
	find . -name "*.o" -delete
	rm -f kernel.elf os.iso bochslog.txt com1.out
	rm -rf iso/

# 删除中间文件时出错则中断
.DELETE_ON_ERROR:
