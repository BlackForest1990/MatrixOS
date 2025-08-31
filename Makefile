# 工具配置
CC = gcc
AS = nasm 
LD = ld

# 编译标志
CFLAGS = -m32 -nostdlib -nostdinc -fno-builtin -fno-stack-protector \
	-nostartfiles -nodefaultlibs -Wall -Wextra -Werror -c \
	-O0 -Iinclude -Ikernel/drivers -Ilib

LDFLAGS = -T link.ld -melf_i386
ASFLAGS = -f elf

# 源文件列表
BOOT_SRC = boot/loader.s
KERNEL_SRCS = kernel/kmain.c \
              kernel/drivers/fb.c \
              kernel/drivers/serial.c \
              kernel/drivers/io.s

# 对象文件列表
BOOT_OBJ = $(BOOT_SRC:.s=.o)
KERNEL_OBJS = $(KERNEL_SRCS:.c=.o)
KERNEL_OBJS := $(KERNEL_OBJS:.s=.o)
OBJECTS = $(BOOT_OBJ) $(KERNEL_OBJS)

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
	rm -rf $(OBJECTS) kernel.elf os.iso iso/
