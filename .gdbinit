# .gdbinit - 极简兼容版 | 适用于老版本 GDB

# 允许加载当前目录的 .gdbinit
add-auto-load-safe-path .

# 基础设置（只保留通用命令）
set confirm off
set pagination off
set print pretty on
set architecture i386

# 一键连接 QEMU
define qstart
    echo \n=== Connecting to QEMU...\n
    target remote :1234
    echo \n=== Registers ===\n
    info registers
    echo \n=== Instructions at \$pc ===\n
    x/5i $pc
end

# 提示
echo \n💡 Type 'qstart' to connect to QEMU\n
