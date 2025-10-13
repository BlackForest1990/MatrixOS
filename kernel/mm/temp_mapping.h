// kernel/mm/temp_mapping.h
#ifndef TEMP_MAPPING_H
#define TEMP_MAPPING_H

#include "types.h"
#include "spinlock.h"

#define TEMP_MAPPING_ADDR 0xC03FF000  // 临时映射的虚拟地址
#define TEMP_MAPPING_COUNT 4          // 支持多个临时映射槽

// 临时映射管理结构
struct temp_mapping_manager {
    spinlock_t lock;
    uint32_t available_slots;         // 位图，标记哪些槽可用
    uint32_t ref_count[TEMP_MAPPING_COUNT]; // 引用计数
};

// 函数声明
void temp_mapping_init(void);
uint32_t temp_map_page(uint32_t physical_addr);
void temp_unmap_page(uint32_t virtual_addr);
void* temp_get_mapping(uint32_t physical_addr);

// 内部页表操作函数
void map_page(uint32_t virtual_addr, uint32_t physical_addr, uint32_t flags);
void unmap_page(uint32_t virtual_addr);

extern struct temp_mapping_manager temp_mgr;

#endif
