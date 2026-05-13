// kernel/fs/vfs.c
#include "vfs.h"
#include "string.h"
#include "serial.h"
#include "fb.h"
#include "kmalloc.h"

static struct filesystem* fs_list = NULL;
static struct file_handle open_files[MAX_OPEN_FILES];

void vfs_init(void) {
    serial_printf(COM1, "VFS: Initializing Virtual Filesystem\n");
    
    memset(open_files, 0, sizeof(open_files));
    fs_list = NULL;
    
    // 初始化标准文件描述符
    open_files[STDIN_FILENO].fs = NULL;
    open_files[STDIN_FILENO].flags = O_RDONLY;
    open_files[STDIN_FILENO].ref_count = 1;
    
    open_files[STDOUT_FILENO].fs = NULL;
    open_files[STDOUT_FILENO].flags = O_WRONLY;
    open_files[STDOUT_FILENO].ref_count = 1;
    
    open_files[STDERR_FILENO].fs = NULL;
    open_files[STDERR_FILENO].flags = O_WRONLY;
    open_files[STDERR_FILENO].ref_count = 1;
}

int vfs_register_fs(const char* name, struct file_operations* ops) {
    if (!name || !ops) return EINVAL;
    
    struct filesystem* new_fs = (struct filesystem*)kmalloc(sizeof(struct filesystem));
    if (!new_fs) return ENOMEM;
    
    strncpy(new_fs->name, name, MAX_FS_NAME - 1);
    new_fs->name[MAX_FS_NAME - 1] = '\0';
    new_fs->ops = ops;
    new_fs->next = fs_list;
    fs_list = new_fs;
    
    serial_printf(COM1, "VFS: Registered filesystem '%s'\n", name);
    return 0;
}

static struct filesystem* vfs_find_registered(const char* fs_name) {
    for (struct filesystem* fs = fs_list; fs; fs = fs->next) {
        if (strcmp(fs->name, fs_name) == 0) {
            return fs;
        }
    }
    return NULL;
}

/*
 * 路径 -> 已注册 filesystem 名（教学版硬编码规则）。
 * 后续可改为：挂载表 + 最长前缀匹配，再 walk 到具体 fs。
 */
static struct filesystem* vfs_resolve_fs(const char* path) {
    const char* name = "ramfs";
    if (strncmp(path, "/dev/", 5) == 0) {
        name = "devfs";
    }
    return vfs_find_registered(name);
}

int vfs_open(const char* path, int flags) {
    if (!path) return EINVAL;
    
    struct filesystem* fs = vfs_resolve_fs(path);
    if (!fs || !fs->ops || !fs->ops->open) return ENOENT;
    
    // 查找空闲文件描述符
    int fd = -1;
    for (int i = 3; i < MAX_OPEN_FILES; i++) {
        if (open_files[i].ref_count == 0) {
            fd = i;
            break;
        }
    }
    if (fd == -1) return EMFILE;
    
    int result = fs->ops->open(path, flags);
    if (result < 0) return result;
    
    open_files[fd].fs = fs;
    open_files[fd].private_data = (void*)result;
    open_files[fd].pos = 0;
    open_files[fd].flags = flags;
    open_files[fd].ref_count = 1;
    
    serial_printf(COM1, "VFS: Opened '%s' -> fd %d\n", path, fd);
    return fd;
}

int vfs_close(int fd) {
    if (fd < 0 || fd >= MAX_OPEN_FILES || open_files[fd].ref_count == 0) {
        return EBADF;
    }
    
    if (--open_files[fd].ref_count > 0) {
        return 0; // 还有引用，不真正关闭
    }
    
    struct file_handle* fh = &open_files[fd];
    if (fh->fs && fh->fs->ops && fh->fs->ops->close) {
        fh->fs->ops->close((int)fh->private_data);
    }
    
    memset(fh, 0, sizeof(struct file_handle));
    serial_printf(COM1, "VFS: Closed fd %d\n", fd);
    return 0;
}

int vfs_read(int fd, void* buf, uint32_t count) {
    if (fd < 0 || fd >= MAX_OPEN_FILES || open_files[fd].ref_count == 0) {
        return EBADF;
    }
    
    struct file_handle* fh = &open_files[fd];
    if (!(fh->flags & O_RDONLY) && !(fh->flags & O_RDWR)) {
        return EACCES;
    }
    
    if (!fh->fs || !fh->fs->ops || !fh->fs->ops->read) {
        return EIO;
    }
    
    int result = fh->fs->ops->read((int)fh->private_data, buf, count);
    if (result > 0) {
        fh->pos += result;
    }
    
    return result;
}

int vfs_write(int fd, const void* buf, uint32_t count) {
    if (fd < 0 || fd >= MAX_OPEN_FILES || open_files[fd].ref_count == 0) {
        return EBADF;
    }
    
    struct file_handle* fh = &open_files[fd];
    if (!(fh->flags & O_WRONLY) && !(fh->flags & O_RDWR)) {
        return EACCES;
    }
    
    if (fd == STDOUT_FILENO || fd == STDERR_FILENO) {
        // 标准输出直接处理
        char* str = (char*)buf;
        for (uint32_t i = 0; i < count; i++) {
            fb_putchar(str[i]);
        }
        return count;
    }
    
    if (!fh->fs || !fh->fs->ops || !fh->fs->ops->write) {
        return EIO;
    }
    
    int result = fh->fs->ops->write((int)fh->private_data, buf, count);
    if (result > 0) {
        fh->pos += result;
    }
    
    return result;
}

int vfs_seek(int fd, int offset, int whence) {
    if (fd < 0 || fd >= MAX_OPEN_FILES || open_files[fd].ref_count == 0) {
        return EBADF;
    }
    
    struct file_handle* fh = &open_files[fd];
    if (!fh->fs || !fh->fs->ops || !fh->fs->ops->seek) {
        return EIO;
    }
    
    return fh->fs->ops->seek((int)fh->private_data, offset, whence);
}

int vfs_ioctl(int fd, int request, void* argp) {
    if (fd < 0 || fd >= MAX_OPEN_FILES || open_files[fd].ref_count == 0) {
        return EBADF;
    }
    
    struct file_handle* fh = &open_files[fd];
    if (!fh->fs || !fh->fs->ops || !fh->fs->ops->ioctl) {
        return EINVAL;
    }
    
    return fh->fs->ops->ioctl((int)fh->private_data, request, argp);
}

int vfs_stat(const char* path, struct file_stat* stat) {
    if (!path || !stat) return EINVAL;
    
    struct filesystem* fs = vfs_resolve_fs(path);
    if (!fs || !fs->ops || !fs->ops->stat) return ENOENT;
    
    return fs->ops->stat(path, stat);
}