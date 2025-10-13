// kernel/mm/kmalloc.h
#ifndef KMALLOC_H
#define KMALLOC_H

#include "types.h"
#include "pmm.h"
#include "spinlock.h"
#include "list.h"

#define KMALLOC_MIN_SIZE   16      // 最小分配大小
#define KMALLOC_MAX_SIZE   (4 * 1024 * 1024) // 最大分配大小 4MB

// 内存块头结构
struct kmalloc_header {
    size_t size;                    // 块大小（包括头部）
    uint8_t is_free;               // 是否空闲
    struct kmalloc_header* next;   // 下一个块
    struct kmalloc_header* prev;   // 前一个块
};

// 堆管理结构
struct heap_manager {
    spinlock_t lock;               // 自旋锁
    struct kmalloc_header* head;   // 堆链表头
    uint32_t total_allocated;      // 总分配字节数
    uint32_t total_freed;          // 总释放字节数
    uint32_t current_usage;        // 当前使用量
};

// 函数声明
void kmalloc_init(void);
void* kmalloc(size_t size);
void kfree(void* ptr);
void* kcalloc(size_t num, size_t size);
void* krealloc(void* ptr, size_t size);
void kmalloc_stats(void);

extern struct heap_manager heap_mgr;

#endif
