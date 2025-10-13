// kernel/mm/pmm.h
#ifndef PMM_H
#define PMM_H

#include "types.h"
#include "spinlock.h"
#include "list.h"

#define MAX_ORDER       11      // 最大阶数：2^11 = 2048 页 = 8MB
#define BUDDY_MAX_ORDER MAX_ORDER

// 伙伴系统空闲链表
struct free_area {
    struct list_head free_list;  // 空闲页链表
    uint32_t nr_free;           // 空闲页块数量
};

// 页帧结构（放在每个空闲页的开头）
struct page {
    struct list_head list;      // 链表节点
    uint32_t order;             // 所属阶数
    uint32_t flags;             // 标志位
};

// 伙伴系统主结构
struct buddy_system {
    spinlock_t lock;                    // 自旋锁
    uint32_t total_pages;              // 总页数
    uint32_t managed_pages;            // 管理的页数  
    uint32_t base_addr;                // 管理区域的基地址
    struct free_area free_area[MAX_ORDER + 1];  // 各阶空闲链表
    uint8_t* bitmap;                   // 位图，标记页的使用情况
};

// 函数声明
void pmm_init(uint32_t memory_start, uint32_t memory_end);
uint32_t pmm_alloc_pages(uint32_t order);
void pmm_free_pages(uint32_t page_addr, uint32_t order);
void pmm_stats(void);

// 工具函数
uint32_t find_buddy_pfn(uint32_t page_idx, uint32_t order);

extern struct buddy_system buddy_sys;

#endif
