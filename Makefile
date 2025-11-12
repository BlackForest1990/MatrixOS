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
         -Iinclude -Ikernel -Ikernel/drivers -Ikernel/interrupts -Ikernel/mm -Ikernel/process -Ikernel/fs -Ikernel/syscall -Ilib \
         -fno-pic \
         -fno-pie \
         -static \
         -fcf-protection=none

LDFLAGS = -T link.ld -melf_i386
ASFLAGS = -f elf32 -g

# C源文件列表 - 添加新的用户模式模块
KERNEL_C_SRCS = kernel/kmain.c \
                kernel/drivers/fb.c \
                kernel/drivers/serial.c \
                kernel/interrupts/idt.c \
                kernel/interrupts/interrupt.c \
				kernel/interrupts/tss.c \
                kernel/interrupts/pic.c \
                kernel/interrupts/keyboard.c \
		kernel/mm/kmalloc.c \
		kernel/mm/pmm.c \
		kernel/mm/temp_mapping.c \
		kernel/mm/vmm.c \
		kernel/mm/test_mm.c \
		kernel/process/process.c \
		kernel/process/loader.c \
		kernel/syscall/syscall.c \
		kernel/fs/vfs.c \
		kernel/fs/ramfs.c \
		kernel/fs/devfs.c \
		kernel/fs/test_fs.c \
		lib/string.c

# 汇编源文件列表 - 添加用户模式相关汇编
KERNEL_ASM_SRCS = kernel/drivers/io.asm \
                  kernel/interrupts/interrupt_asm.asm \
                  kernel/process/switch.asm \
                  kernel/syscall/syscall_asm.asm \
				  kernel/interrupts/tss_asm.asm 

# 引导加载程序汇编文件
BOOT_SRC = boot/loader.asm

# 对象文件列表
BOOT_OBJ = $(BOOT_SRC:.asm=.o)
KERNEL_C_OBJS = $(KERNEL_C_SRCS:.c=.o)
KERNEL_ASM_OBJS = $(KERNEL_ASM_SRCS:.asm=.o)
OBJECTS = $(BOOT_OBJ) $(KERNEL_C_OBJS) $(KERNEL_ASM_OBJS)

# 用户程序
USER_PROGRAMS = user/programs/hello.asm user/programs/file_test.asm
USER_BINS = $(patsubst user/programs/%.asm,user/build/%.bin,$(USER_PROGRAMS))

# 默认目标
all: user-programs kernel.elf

# 内核ELF文件
kernel.elf: $(OBJECTS)
	$(LD) $(LDFLAGS) $(OBJECTS) -o kernel.elf

# 编译用户程序
user-programs: $(USER_PROGRAMS)
	@mkdir -p user/build
	$(foreach prog,$(USER_PROGRAMS),\
		$(AS) -f bin $(prog) -o $(patsubst user/programs/%.asm,user/build/%.bin,$(prog));)


# 创建ISO镜像（包含用户程序模块）
os.iso: kernel.elf user-programs
	@mkdir -p iso/boot/grub
	cp kernel.elf iso/boot/kernel.elf
	cp $(USER_BINS) iso/boot/
	@echo 'set timeout=0' > iso/boot/grub/grub.cfg
	@echo 'set default=0' >> iso/boot/grub/grub.cfg
	@echo '' >> iso/boot/grub/grub.cfg
	@echo 'menuentry "MatrixOS with File System" {' >> iso/boot/grub/grub.cfg
	@echo '  multiboot /boot/kernel.elf' >> iso/boot/grub/grub.cfg
	@echo '  module /boot/hello.bin hello' >> iso/boot/grub/grub.cfg
	@echo '  module /boot/file_test.bin file_test' >> iso/boot/grub/grub.cfg
	@echo '}' >> iso/boot/grub/grub.cfg
	grub-mkrescue -o os.iso iso

# 快速运行 QEMU（直接加载内核）- 不带模块
qemu: kernel.elf
	qemu-system-i386 -kernel kernel.elf -serial stdio -no-reboot -no-shutdown

# 使用ISO运行QEMU（包含用户程序模块）
qemu-iso: os.iso
	qemu-system-i386 -cdrom os.iso -serial stdio -no-reboot -no-shutdown

# ========== 新增：调试目标（包含用户模块）==========
# 调试模式 - 使用ISO（推荐，包含用户模块）
debug-iso: os.iso
	qemu-system-i386 -cdrom os.iso \
		-s -S \
		-serial stdio \
		-no-reboot \
		-no-shutdown

# 调试模式 - 直接加载内核和模块（使用initrd）
debug-modules: kernel.elf user-programs
	qemu-system-i386 -kernel kernel.elf \
		-initrd $(USER_BINS) \
		-append "modules=hello,file_test" \
		-s -S \
		-serial stdio \
		-no-reboot \
		-no-shutdown

# 调试模式 - 传统方式（不包含模块）
debug-qemu: kernel.elf
	qemu-system-i386 -kernel kernel.elf \
		-s -S \
		-serial stdio \
		-no-reboot \
		-no-shutdown

# 运行bochs
run: os.iso
	bochs -f bochsrc.txt -q

# 编译规则
%.o: %.c
	$(CC) $(CFLAGS) $< -o $@

%.o: %.asm
	$(AS) $(ASFLAGS) $< -o $@

# 清理
clean:
	find . -name "*.o" -delete
	rm -f kernel.elf os.iso bochslog.txt com1.out $(USER_BIN)
	rm -rf iso/ user/build/

# 显示构建信息
info:
	@echo "=== Build Information ==="
	@echo "C Sources: $(words $(KERNEL_C_SRCS)) files"
	@echo "ASM Sources: $(words $(KERNEL_ASM_SRCS)) files"
	@echo "User Programs: $(USER_PROGRAMS)"
	@echo "User Binaries: $(USER_BINS)"
	@echo "=== Debug Targets ==="
	@echo "debug-iso     : Debug with ISO (includes user modules)"
	@echo "debug-modules : Debug with direct module loading"
	@echo "debug-qemu    : Debug kernel only (no modules)"

# 快速构建（不清理）
quick: $(OBJECTS)
	$(LD) $(LDFLAGS) $(OBJECTS) -o kernel.elf

# 删除中间文件时出错则中断
.DELETE_ON_ERROR:

.PHONY: all clean qemu debug-qemu debug-iso debug-modules run user-programs qemu-iso info quick