// kernel/mm/temp_mapping.c
#include "temp_mapping.h"
#include "pmm.h"
#include "serial.h"
#include "string.h"

struct temp_mapping_manager temp_mgr;

/*初始化临时页面映射系统，为内核提供快速映射物理页到固定虚拟地址的能力*/
void temp_mapping_init(void) {
    serial_printf(COM1, "TEMP_MAPPING: Initializing temporary mapping framework...\n");
    
    spinlock_init(&temp_mgr.lock);
    temp_mgr.available_slots = (1 << TEMP_MAPPING_COUNT) - 1; // 所有槽初始可用
    
    for (int i = 0; i < TEMP_MAPPING_COUNT; i++) {
        temp_mgr.ref_count[i] = 0;
    }
    
    serial_printf(COM1, "TEMP_MAPPING: %d slots at 0x%x\n", 
                 TEMP_MAPPING_COUNT, TEMP_MAPPING_ADDR);
}

uint32_t temp_map_page(uint32_t physical_addr) {
    if (physical_addr & 0xFFF) {
        serial_printf(COM1, "TEMP_MAPPING: Physical address not page-aligned: 0x%x\n", physical_addr);
        return 0;
    }
    
    spinlock_acquire(&temp_mgr.lock);
    
    // 查找可用的映射槽
    int slot = -1;
    for (int i = 0; i < TEMP_MAPPING_COUNT; i++) {
        if (temp_mgr.available_slots & (1 << i)) {
            slot = i;
            break;
        }
    }
    
    if (slot == -1) {
        spinlock_release(&temp_mgr.lock);
        serial_printf(COM1, "TEMP_MAPPING: No available mapping slots\n");
        return 0;
    }
    
    // 计算虚拟地址
    uint32_t virtual_addr = TEMP_MAPPING_ADDR - (slot * PAGE_SIZE);
    
    // 映射物理页到虚拟地址
    map_page(virtual_addr, physical_addr, 0x03); // Present + Writable
    
    // 更新管理状态
    temp_mgr.available_slots &= ~(1 << slot);
    temp_mgr.ref_count[slot]++;
    
    spinlock_release(&temp_mgr.lock);
    
    serial_printf(COM1, "TEMP_MAPPING: Mapped 0x%x -> 0x%x (slot %d)\n", 
                 physical_addr, virtual_addr, slot);
    
    return virtual_addr;
}

void temp_unmap_page(uint32_t virtual_addr) {
    // 验证地址是否在临时映射区域
    if (virtual_addr < TEMP_MAPPING_ADDR - (TEMP_MAPPING_COUNT - 1) * PAGE_SIZE || 
        virtual_addr > TEMP_MAPPING_ADDR) {
        serial_printf(COM1, "TEMP_MAPPING: Invalid temporary mapping address: 0x%x\n", virtual_addr);
        return;
    }
    
    spinlock_acquire(&temp_mgr.lock);
    
    // 计算槽索引
    int slot = (TEMP_MAPPING_ADDR - virtual_addr) / PAGE_SIZE;
    
    if (slot < 0 || slot >= TEMP_MAPPING_COUNT) {
        spinlock_release(&temp_mgr.lock);
        serial_printf(COM1, "TEMP_MAPPING: Invalid slot index: %d\n", slot);
        return;
    }
    
    if (temp_mgr.ref_count[slot] == 0) {
        spinlock_release(&temp_mgr.lock);
        serial_printf(COM1, "TEMP_MAPPING: Slot %d already unmapped\n", slot);
        return;
    }
    
    // 减少引用计数
    temp_mgr.ref_count[slot]--;
    
    // 如果引用计数为0，取消映射
    if (temp_mgr.ref_count[slot] == 0) {
        unmap_page(virtual_addr);
        temp_mgr.available_slots |= (1 << slot);
        
        serial_printf(COM1, "TEMP_MAPPING: Unmapped 0x%x (slot %d)\n", virtual_addr, slot);
    } else {
        serial_printf(COM1, "TEMP_MAPPING: Decremented refcount for 0x%x (slot %d, refs: %d)\n", 
                     virtual_addr, slot, temp_mgr.ref_count[slot]);
    }
    
    spinlock_release(&temp_mgr.lock);
}

void* temp_get_mapping(uint32_t physical_addr) {
    uint32_t virtual_addr = temp_map_page(physical_addr);
    return (void*)virtual_addr;
}

// 内部函数：映射页
void map_page(uint32_t virtual_addr, uint32_t physical_addr, uint32_t flags) {
    uint32_t page_dir_index = virtual_addr >> 22;
    uint32_t page_tab_index = (virtual_addr >> 12) & 0x3FF;
    
    // 获取页目录（假设内核页目录在 0xFFFFF000）
    uint32_t* page_dir = (uint32_t*)0xFFFFF000;
    
    // 检查页表是否存在
    if (!(page_dir[page_dir_index] & 0x1)) {
        // 页表不存在，需要分配新的页表
        uint32_t new_page_table = pmm_alloc_pages(0);
        
        // 临时映射新页表来初始化它
        uint32_t temp_mapping = temp_map_page(new_page_table);
        uint32_t* page_table = (uint32_t*)temp_mapping;
        
        // 清零页表
        memset(page_table, 0, PAGE_SIZE);
        
        // 设置页目录项
        page_dir[page_dir_index] = new_page_table | flags;
        
        // 取消临时映射
        temp_unmap_page(temp_mapping);
    }
    
    // 现在页表存在，映射目标页
    uint32_t* page_table = (uint32_t*)(0xFFC00000 | (page_dir_index << 12));
    page_table[page_tab_index] = physical_addr | flags;
    
    // 刷新TLB
    asm volatile("invlpg (%0)" : : "r" (virtual_addr) : "memory");
}

// 内部函数：取消页映射
void unmap_page(uint32_t virtual_addr) {
    uint32_t page_dir_index = virtual_addr >> 22;
    uint32_t page_tab_index = (virtual_addr >> 12) & 0x3FF;
    
    uint32_t* page_table = (uint32_t*)(0xFFC00000 | (page_dir_index << 12));
    page_table[page_tab_index] = 0; // 清除页表项
    
    // 刷新TLB
    asm volatile("invlpg (%0)" : : "r" (virtual_addr) : "memory");
}
