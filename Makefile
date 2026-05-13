# 工具配置
# Linux: 系统 gcc/ld + -m32 / -melf_i386
# macOS: Apple 自带工具链生成 Mach-O，无法链接本仓库的 ELF 内核；请用 Homebrew 的 i686-elf-* 交叉工具链
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
  CC := i686-elf-gcc
  LD := i686-elf-ld
  KERNEL_ARCH_CFLAGS :=
else
  CC := gcc
  LD := ld
  KERNEL_ARCH_CFLAGS := -m32
endif

AS = nasm

# 编译标志
CFLAGS = $(KERNEL_ARCH_CFLAGS) \
         -std=gnu11 \
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

# 演示：CONFIG_DEMO_STARTUP=0 关闭 RAMFS 串口演示与用户进程演示（仍初始化内核与 VFS）
CONFIG_DEMO_STARTUP ?= 1
ifneq ($(CONFIG_DEMO_STARTUP),0)
CFLAGS += -DCONFIG_DEMO_STARTUP=1
else
CFLAGS += -DCONFIG_DEMO_STARTUP=0
endif

# C源文件列表 - 添加新的用户模式模块
KERNEL_C_SRCS = kernel/kmain.c \
                kernel/boot.c \
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
comma := ,
empty :=
space := $(empty) $(empty)
USER_INITRD := $(subst $(space),$(comma),$(USER_BINS))

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

# 使用 ISO 运行 QEMU（包含用户程序模块）
qemu-iso: os.iso
	qemu-system-i386 -cdrom os.iso -serial stdio -no-reboot -no-shutdown

# 无 ISO：Multiboot 内核 + 多个模块（QEMU 要求 -initrd 内逗号分隔）
qemu-modules: kernel.elf user-programs
	qemu-system-i386 -kernel kernel.elf \
		-initrd $(USER_INITRD) \
		-append "modules=hello,file_test" \
		-serial stdio \
		-no-reboot \
		-no-shutdown

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
		-initrd $(USER_INITRD) \
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
	rm -f kernel.elf os.iso bochslog.txt com1.out $(USER_BINS)
	rm -rf iso/ user/build/

# 显示构建信息
info:
	@echo "=== Build Information ==="
	@echo "C Sources: $(words $(KERNEL_C_SRCS)) files"
	@echo "ASM Sources: $(words $(KERNEL_ASM_SRCS)) files"
	@echo "User Programs: $(USER_PROGRAMS)"
	@echo "User Binaries: $(USER_BINS)"
	@echo "=== Debug Targets ==="
	@echo "debug-iso      : Debug with ISO (includes user modules)"
	@echo "debug-modules  : Debug with direct module loading"
	@echo "debug-qemu     : Debug kernel only (no modules)"
	@echo "qemu-modules   : Run kernel + user modules (no ISO, no GDB wait)"
	@echo "CONFIG_DEMO_STARTUP=0 : Skip FS demo + user program demo (include/boot_config.h)"

# 快速构建（不清理）
quick: $(OBJECTS)
	$(LD) $(LDFLAGS) $(OBJECTS) -o kernel.elf

# 删除中间文件时出错则中断
.DELETE_ON_ERROR:

.PHONY: all clean qemu debug-qemu debug-iso debug-modules run user-programs qemu-iso qemu-modules info quick