// kernel/fs/ramfs.h
#ifndef RAMFS_H
#define RAMFS_H

#include "types.h"
#include "vfs.h"


#define MAX_RAMFS_FILES 64


struct ramfs_file {
    char name[MAX_PATH];
    uint8_t* data;
    uint32_t size;
    uint32_t mode;
    uint32_t type;
};

struct ramfs_private {
    struct ramfs_file* file;
    uint32_t pos;
};

void ramfs_init(void);
struct file_operations* ramfs_get_ops(void);

#endif