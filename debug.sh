#!/bin/bash
# debug.sh - 一键启动 QEMU + GDB 调试

echo "🚀 构建内核..."
make debug-qemu &
QEMU_PID=$!

# 等待 QEMU 启动
echo "⏳ 等待 QEMU 启动 GDB server..."
sleep 1

# 检查是否成功启动
if ! kill -0 $QEMU_PID 2>/dev/null; then
    echo "❌ QEMU 启动失败"
    exit 1
fi

echo "🎯 连接 GDB 调试器..."

# 启动 GDB 并发送命令
gdb kernel.elf -ex qstart

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
