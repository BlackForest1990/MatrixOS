#include "vmm.h"
#include "pmm.h"
#include "temp_mapping.h"
#include "serial.h"
#include "string.h"

uint32_t vmm_create_user_space(void) {
    uint32_t user_page_dir = pmm_alloc_pages(0);
    if (!user_page_dir) return 0;
    
    uint32_t temp_mapping = temp_map_page(user_page_dir);
    uint32_t* page_dir = (uint32_t*)temp_mapping;
    memset(page_dir, 0, PAGE_SIZE);
    
    uint32_t* kernel_page_dir = (uint32_t*)0xFFFFF000;
    for (int i = 768; i < 1024; i++) {
        page_dir[i] = kernel_page_dir[i];
    }
    
    temp_unmap_page(temp_mapping);
    return user_page_dir;
}

void vmm_destroy_user_space(uint32_t page_dir) {
    if (!page_dir) return;
    pmm_free_pages(page_dir, 0);
}

int vmm_load_user_program(uint32_t page_dir, uint32_t binary_phys, uint32_t size) {
    // 分配用户代码页
    uint32_t user_code_page = pmm_alloc_pages(0);
    if (!user_code_page) {
        serial_printf(COM1, "VMM: Failed to allocate user code page\n");
        return 0;
    }
    
    // 分配用户栈页
    uint32_t user_stack_page = pmm_alloc_pages(0);
    if (!user_stack_page) {
        serial_printf(COM1, "VMM: Failed to allocate user stack page\n");
        pmm_free_pages(user_code_page, 0);
        return 0;
    }
    
    uint32_t temp_page_dir = temp_map_page(page_dir);
    uint32_t* page_dir_ptr = (uint32_t*)temp_page_dir;
    
    // 创建页表用于用户代码（虚拟地址 0x00000000）
    uint32_t code_page_table = pmm_alloc_pages(0);
    if (!code_page_table) {
        serial_printf(COM1, "VMM: Failed to allocate code page table\n");
        temp_unmap_page(temp_page_dir);
        pmm_free_pages(user_code_page, 0);
        pmm_free_pages(user_stack_page, 0);
        return 0;
    }
    
    // 创建页表用于用户栈（虚拟地址 0xBFFFF000）
    uint32_t stack_page_table = pmm_alloc_pages(0);
    if (!stack_page_table) {
        serial_printf(COM1, "VMM: Failed to allocate stack page table\n");
        temp_unmap_page(temp_page_dir);
        pmm_free_pages(user_code_page, 0);
        pmm_free_pages(user_stack_page, 0);
        pmm_free_pages(code_page_table, 0);
        return 0;
    }
    
    serial_printf(COM1, "VMM: Setting up page tables:\n");
    serial_printf(COM1, "  Code page: 0x%x, Code page table: 0x%x\n", user_code_page, code_page_table);
    serial_printf(COM1, "  Stack page: 0x%x, Stack page table: 0x%x\n", user_stack_page, stack_page_table);
    
    // 设置代码映射：虚拟地址 0x00000000
    page_dir_ptr[0] = code_page_table | 0x07;
    uint32_t temp_code_table = temp_map_page(code_page_table);
    uint32_t* code_table = (uint32_t*)temp_code_table;
    memset(code_table, 0, PAGE_SIZE);
    code_table[0] = user_code_page | 0x07; // 虚拟地址 0x00000000
    temp_unmap_page(temp_code_table);
    
    // 设置栈映射：虚拟地址 0xBFFFF000 (0x3FE * 4MB = 0xBFF80000, 页表项 0x3FF = 0xBFFFF000)
    page_dir_ptr[0x3FE] = stack_page_table | 0x07;
    uint32_t temp_stack_table = temp_map_page(stack_page_table);
    uint32_t* stack_table = (uint32_t*)temp_stack_table;
    memset(stack_table, 0, PAGE_SIZE);
    stack_table[0x3FF] = user_stack_page | 0x07; // 虚拟地址 0xBFFFF000
    temp_unmap_page(temp_stack_table);
    
    temp_unmap_page(temp_page_dir);
    
    // 复制用户程序到用户代码页
    uint32_t temp_src = temp_map_page(binary_phys);
    uint32_t temp_dst = temp_map_page(user_code_page);
    
    memcpy((void*)temp_dst, (void*)temp_src, size);
    
    serial_printf(COM1, "VMM: User program loaded:\n");
    serial_printf(COM1, "  Code at 0x00000000 -> 0x%x\n", user_code_page);
    serial_printf(COM1, "  Stack at 0xBFFFF000 -> 0x%x\n", user_stack_page);
    
    temp_unmap_page(temp_src);
    temp_unmap_page(temp_dst);
    
    return 1;
}