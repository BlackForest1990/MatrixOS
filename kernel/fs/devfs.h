// kernel/fs/devfs.h
#ifndef DEVFS_H
#define DEVFS_H

#include "types.h"

void devfs_init(void);
struct file_operations* devfs_get_ops(void);

// 设备号定义
#define DEVICE_CONSOLE   1
#define DEVICE_NULL      2
#define DEVICE_ZERO      3

#endif