// kernel/fs/vfs.h
#ifndef VFS_H
#define VFS_H

#include "types.h"

#define MAX_FS_NAME 16
#define MAX_PATH 256
#define MAX_OPEN_FILES 32

// 文件打开标志
#define O_RDONLY    0x0001
#define O_WRONLY    0x0002  
#define O_RDWR      0x0003
#define O_CREAT     0x0004
#define O_APPEND    0x0008
#define O_TRUNC     0x0010

// 文件类型
#define FT_UNKNOWN  0
#define FT_REGULAR  1
#define FT_DIRECTORY 2
#define FT_CHAR_DEVICE 3

// 标准文件描述符
#define STDIN_FILENO  0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

// 错误代码
#define ENOENT      -1  // 文件不存在
#define EACCES      -2  // 权限不足  
#define EMFILE      -3  // 文件描述符用完
#define EBADF       -4  // 错误的文件描述符
#define EINVAL      -5  // 无效参数
#define EIO         -6  // I/O错误

struct file_stat {
    uint32_t size;
    uint32_t mode;
    uint32_t type;
    uint32_t inode;
};

struct filesystem {
    char name[MAX_FS_NAME];
    struct file_operations* ops;
    struct filesystem* next;
};

struct file_handle {
    struct filesystem* fs;
    void* private_data;  // 文件系统私有数据
    uint32_t pos;
    uint32_t flags;
    int ref_count;
};

struct file_operations {
    int (*open)(const char* path, int flags);
    int (*close)(int fd);
    int (*read)(int fd, void* buf, uint32_t count);
    int (*write)(int fd, const void* buf, uint32_t count);
    int (*seek)(int fd, int offset, int whence);
    int (*ioctl)(int fd, int request, void* argp);
    int (*stat)(const char* path, struct file_stat* stat);
};

// VFS 公共接口
void vfs_init(void);
int vfs_register_fs(const char* name, struct file_operations* ops);
int vfs_open(const char* path, int flags);
int vfs_close(int fd);
int vfs_read(int fd, void* buf, uint32_t count);
int vfs_write(int fd, const void* buf, uint32_t count);
int vfs_seek(int fd, int offset, int whence);
int vfs_ioctl(int fd, int request, void* argp);
int vfs_stat(const char* path, struct file_stat* stat);

#endif