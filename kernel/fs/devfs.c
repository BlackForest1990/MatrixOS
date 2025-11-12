// kernel/fs/devfs.c
#include "devfs.h"
#include "vfs.h"
#include "fb.h"
#include "keyboard.h"
#include "serial.h"
#include "string.h"

static struct file_operations devfs_ops;

struct devfs_private {
    int device;
    uint32_t pos;
};

static struct devfs_private devfs_handles[MAX_OPEN_FILES];

struct file_operations* devfs_get_ops(void) {
    return &devfs_ops;
}

static int devfs_resolve_device(const char* path) {
    if (strcmp(path, "/dev/console") == 0) return DEVICE_CONSOLE;
    if (strcmp(path, "/dev/null") == 0) return DEVICE_NULL;
    if (strcmp(path, "/dev/zero") == 0) return DEVICE_ZERO;
    return -1;
}

int devfs_open(const char* path, int flags) {
    (void)flags;
    int device = devfs_resolve_device(path);
    if (device == -1) return ENOENT;
    
    // 查找空闲句柄
    int handle = -1;
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        if (devfs_handles[i].device == 0) {
            handle = i;
            break;
        }
    }
    if (handle == -1) return EMFILE;
    
    devfs_handles[handle].device = device;
    devfs_handles[handle].pos = 0;
    
    serial_printf(COM1, "DEVFS: Opened '%s' -> device %d, handle %d\n", 
                 path, device, handle);
    return handle;
}

int devfs_close(int handle) {
    if (handle < 0 || handle >= MAX_OPEN_FILES || devfs_handles[handle].device == 0) {
        return EBADF;
    }
    
    devfs_handles[handle].device = 0;
    devfs_handles[handle].pos = 0;
    return 0;
}

int devfs_read(int handle, void* buf, uint32_t count) {
    if (handle < 0 || handle >= MAX_OPEN_FILES || devfs_handles[handle].device == 0) {
        return EBADF;
    }
    
    int device = devfs_handles[handle].device;
    char* buffer = (char*)buf;
    
    switch (device) {
        case DEVICE_CONSOLE:
            // 从键盘读取
            {
                uint32_t read_count = 0;
                while (read_count < count) {
                    char c = keyboard_get_char();
                    if (c) {
                        buffer[read_count++] = c;
                        if (c == '\n') break;
                    } else {
                        break; // 无更多输入
                    }
                }
                return read_count;
            }
            
        case DEVICE_NULL:
            return 0; // 总是返回EOF
            
        case DEVICE_ZERO:
            // 返回零字节
            memset(buf, 0, count);
            return count;
            
        default:
            return EIO;
    }
}

int devfs_write(int handle, const void* buf, uint32_t count) {
    if (handle < 0 || handle >= MAX_OPEN_FILES || devfs_handles[handle].device == 0) {
        return EBADF;
    }
    
    int device = devfs_handles[handle].device;
    const char* buffer = (const char*)buf;
    
    switch (device) {
        case DEVICE_CONSOLE:
            // 写入控制台
            for (uint32_t i = 0; i < count; i++) {
                fb_putchar(buffer[i]);
            }
            return count;
            
        case DEVICE_NULL:
            return count; // 数据被丢弃
            
        case DEVICE_ZERO:
            return count; // 写入 /dev/zero 成功但无效果
            
        default:
            return EIO;
    }
}

int devfs_seek(int handle, int offset, int whence) {
    // 设备文件通常不支持seek
    (void)handle;
    (void)offset;
    (void)whence;
    return EINVAL;
}

int devfs_ioctl(int handle, int request, void* argp) {
    if (handle < 0 || handle >= MAX_OPEN_FILES || devfs_handles[handle].device == 0) {
        return EBADF;
    }
    
    // 简单的控制台控制
    if (devfs_handles[handle].device == DEVICE_CONSOLE) {
        switch (request) {
            case 1: // 清屏
                fb_clear();
                return 0;
            case 2: // 获取光标位置
                if (argp) {
                    *(uint32_t*)argp = 0; // 简化实现
                    return 0;
                }
                break;
        }
    }
    
    return EINVAL;
}

int devfs_stat(const char* path, struct file_stat* stat) {
    int device = devfs_resolve_device(path);
    if (device == -1) return ENOENT;
    
    stat->size = 0; // 设备文件没有大小
    stat->mode = 0666; // 所有用户可读写
    stat->type = FT_CHAR_DEVICE;
    stat->inode = device;
    
    return 0;
}

void devfs_init(void) {
    serial_printf(COM1, "DEVFS: Initializing device filesystem\n");
    
    memset(devfs_handles, 0, sizeof(devfs_handles));
    
    // 设置操作函数
    devfs_ops.open = devfs_open;
    devfs_ops.close = devfs_close;
    devfs_ops.read = devfs_read;
    devfs_ops.write = devfs_write;
    devfs_ops.seek = devfs_seek;
    devfs_ops.ioctl = devfs_ioctl;
    devfs_ops.stat = devfs_stat;
    
    // 注册到VFS
    vfs_register_fs("devfs", &devfs_ops);
}