#!/bin/bash
# debug.sh - 一键启动 QEMU + GDB 调试（支持模块）

echo "🚀 构建内核和用户模块..."
make user-programs
make os.iso

echo "⏳ 启动 QEMU（包含用户模块）..."
qemu-system-i386 -cdrom os.iso -s -S -serial stdio -no-reboot -no-shutdown &
QEMU_PID=$!

# 等待 QEMU 启动
echo "⏳ 等待 QEMU 启动 GDB server..."
sleep 2

# 检查是否成功启动
if ! kill -0 $QEMU_PID 2>/dev/null; then
    echo "❌ QEMU 启动失败"
    exit 1
fi

echo "🎯 连接 GDB 调试器..."

# 创建简化的 GDB 命令文件
cat > .temp_gdb_commands << 'EOF'
target remote :1234
file kernel.elf

# 用户模式关键断点
break switch_to_user_mode
break syscall_entry
break syscall_handler

# 模块加载断点  
break loader_init
break process_create_from_module

echo "=== GDB 调试就绪 ==="
echo "断点已设置:"
echo "  switch_to_user_mode - 用户模式切换"
echo "  syscall_entry - 系统调用入口"
echo "  loader_init - 模块加载器"
echo "  process_create_from_module - 进程创建"
echo ""
echo "输入 'c' 开始执行"
EOF

# 启动 GDB
gdb -x .temp_gdb_commands

# 清理临时文件
rm -f .temp_gdb_commands

# 询问是否退出 QEMU
echo -n "🛑 调试结束。是否关闭 QEMU？(y/N): "
read answer
if [[ "$answer" =~ ^[Yy]$ ]]; then
    kill $QEMU_PID 2>/dev/null
    echo "👋 QEMU 已关闭"
else
    echo "💡 QEMU 继续运行，PID: $QEMU_PID"
    echo "   手动关闭请执行: kill $QEMU_PID"
fi