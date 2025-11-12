// kernel/fs/ramfs.c
#include "ramfs.h"
#include "vfs.h"
#include "string.h"
#include "serial.h"
#include "loader.h"

static struct ramfs_file files[MAX_RAMFS_FILES];
static uint32_t file_count = 0;
static struct file_operations ramfs_ops;

static struct ramfs_private ramfs_handles[MAX_OPEN_FILES];

struct file_operations* ramfs_get_ops(void) {
    return &ramfs_ops;
}

static struct ramfs_file* ramfs_find_file(const char* path) {
    // 跳过前导 '/'
    const char* filename = path;
    while (*filename == '/') filename++;
    
    for (uint32_t i = 0; i < file_count; i++) {
        if (strcmp(files[i].name, filename) == 0) {
            return &files[i];
        }
    }
    return NULL;
}

int ramfs_open(const char* path, int flags) {
    struct ramfs_file* file = ramfs_find_file(path);
    if (!file) return ENOENT;
    
    // 检查权限
    if ((flags & O_WRONLY) && (file->mode & 0222) == 0) {
        return EACCES; // 尝试写入只读文件
    }
    
    // 查找空闲句柄
    int handle = -1;
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        if (ramfs_handles[i].file == NULL) {
            handle = i;
            break;
        }
    }
    if (handle == -1) return EMFILE;
    
    ramfs_handles[handle].file = file;
    ramfs_handles[handle].pos = 0;
    
    serial_printf(COM1, "RAMFS: Opened '%s' -> handle %d\n", path, handle);
    return handle;
}

int ramfs_close(int handle) {
    if (handle < 0 || handle >= MAX_OPEN_FILES || !ramfs_handles[handle].file) {
        return EBADF;
    }
    
    ramfs_handles[handle].file = NULL;
    ramfs_handles[handle].pos = 0;
    return 0;
}

int ramfs_read(int handle, void* buf, uint32_t count) {
    if (handle < 0 || handle >= MAX_OPEN_FILES || !ramfs_handles[handle].file) {
        return EBADF;
    }
    
    struct ramfs_private* priv = &ramfs_handles[handle];
    struct ramfs_file* file = priv->file;
    
    if (priv->pos >= file->size) return 0;
    
    uint32_t to_read = count;
    if (priv->pos + to_read > file->size) {
        to_read = file->size - priv->pos;
    }
    
    memcpy(buf, file->data + priv->pos, to_read);
    priv->pos += to_read;
    
    return to_read;
}

int ramfs_write(int handle, const void* buf, uint32_t count) {
    // RAMFS 是只读的
    (void)handle;
    (void)buf;
    (void)count;
    return EACCES;
}

int ramfs_seek(int handle, int offset, int whence) {
    if (handle < 0 || handle >= MAX_OPEN_FILES || !ramfs_handles[handle].file) {
        return EBADF;
    }
    
    struct ramfs_private* priv = &ramfs_handles[handle];
    struct ramfs_file* file = priv->file;
    
    switch (whence) {
        case 0: // SEEK_SET
            priv->pos = offset;
            break;
        case 1: // SEEK_CUR
            priv->pos += offset;
            break;
        case 2: // SEEK_END
            priv->pos = file->size + offset;
            break;
        default:
            return EINVAL;
    }
    
    // 确保位置在有效范围内
    if (priv->pos > file->size) priv->pos = file->size;
    
    return priv->pos;
}

int ramfs_ioctl(int handle, int request, void* argp) {
    // RAMFS 不支持 ioctl
    (void)handle;
    (void)request;
    (void)argp;
    return EINVAL;
}

int ramfs_stat(const char* path, struct file_stat* stat) {
    struct ramfs_file* file = ramfs_find_file(path);
    if (!file) return ENOENT;
    
    stat->size = file->size;
    stat->mode = file->mode;
    stat->type = file->type;
    stat->inode = (uint32_t)file; // 使用地址作为inode
    
    return 0;
}
void ramfs_init(void) {
    serial_printf(COM1, "RAMFS: Initializing RAM filesystem\n");
    
    memset(files, 0, sizeof(files));
    memset(ramfs_handles, 0, sizeof(ramfs_handles));
    file_count = 0;
    
    // 设置操作函数
    ramfs_ops.open = ramfs_open;
    ramfs_ops.close = ramfs_close;
    ramfs_ops.read = ramfs_read;
    ramfs_ops.write = ramfs_write;
    ramfs_ops.seek = ramfs_seek;
    ramfs_ops.ioctl = ramfs_ioctl;
    ramfs_ops.stat = ramfs_stat;
    
    // 从loader注册所有模块为文件
    extern uint32_t loader_get_module_count(void);
    extern module_info_t* loader_get_module(uint32_t index);
    
    uint32_t mod_count = loader_get_module_count();
    for (uint32_t i = 0; i < mod_count && file_count < MAX_RAMFS_FILES; i++) {
        module_info_t* mod = loader_get_module(i);
        if (mod) {
            strncpy(files[file_count].name, mod->name, MAX_PATH - 1);
            files[file_count].name[MAX_PATH - 1] = '\0';
            files[file_count].data = (uint8_t*)mod->start;
            files[file_count].size = mod->size;
            files[file_count].mode = 0444; // 只读
            files[file_count].type = FT_REGULAR;
            file_count++;
            
            serial_printf(COM1, "RAMFS: Registered file '%s' (%d bytes)\n", 
                         mod->name, mod->size);
        }
    }
    
    // 注册到VFS
    vfs_register_fs("ramfs", &ramfs_ops);
}