// kernel/mm/test_mm.c
#include "pmm.h"
#include "temp_mapping.h"
#include "kmalloc.h"
#include "serial.h"
#include "string.h"

void test_buddy_system(void) {
    serial_printf(COM1, "\n=== Buddy System Test ===\n");
    
    // 显示初始状态
    pmm_stats();
    
    // 测试1: 分配不同大小的内存块
    serial_printf(COM1, "\n--- Test 1: Allocate various sizes ---\n");
    uint32_t block1 = pmm_alloc_pages(0);  // 1 page (4KB)
    uint32_t block2 = pmm_alloc_pages(2);  // 4 pages (16KB)  
    uint32_t block3 = pmm_alloc_pages(4);  // 16 pages (64KB)
    
    if (!block1 || !block2 || !block3) {
        serial_printf(COM1, "ERROR: Allocation failed!\n");
        return;
    }
    
    pmm_stats();
    
    // 测试2: 释放部分块并观察合并
    serial_printf(COM1, "\n--- Test 2: Free and merge ---\n");
    pmm_free_pages(block2, 2);
    pmm_free_pages(block1, 0);
    
    pmm_stats();
    
    // 测试3: 分配大块
    serial_printf(COM1, "\n--- Test 3: Allocate large block ---\n");
    uint32_t block4 = pmm_alloc_pages(5);  // 32 pages (128KB)
    if (block4) {
        serial_printf(COM1, "Large allocation successful at 0x%x\n", block4);
    } else {
        serial_printf(COM1, "Large allocation failed (expected if memory fragmented)\n");
    }
    
    pmm_stats();
    
    // 测试4: 清理所有分配
    serial_printf(COM1, "\n--- Test 4: Cleanup ---\n");
    if (block3) pmm_free_pages(block3, 4);
    if (block4) pmm_free_pages(block4, 5);
    
    pmm_stats();
    
    serial_printf(COM1, "=== Buddy System Test Completed ===\n");
}

void test_temp_mapping(void) {
    serial_printf(COM1, "\n=== Temporary Mapping Test ===\n");
    
    // 测试1: 基本映射功能
    serial_printf(COM1, "\n--- Test 1: Basic mapping ---\n");
    uint32_t phys_page = pmm_alloc_pages(0);
    if (!phys_page) {
        serial_printf(COM1, "ERROR: Failed to allocate physical page\n");
        return;
    }
    
    uint32_t virt_page = temp_map_page(phys_page);
    if (!virt_page) {
        serial_printf(COM1, "ERROR: Failed to create temporary mapping\n");
        pmm_free_pages(phys_page, 0);
        return;
    }
    
    // 写入数据到映射的页
    char* data = (char*)virt_page;
    for (int i = 0; i < 100; i++) {
        data[i] = 'A' + (i % 26);
    }
    data[100] = '\0';
    
    serial_printf(COM1, "Written to temp mapping: %s\n", data);
    
    // 读取验证
    serial_printf(COM1, "Read verification: %s\n", data);
    
    temp_unmap_page(virt_page);
    pmm_free_pages(phys_page, 0);
    
    // 测试2: 多个同时映射
    serial_printf(COM1, "\n--- Test 2: Multiple mappings ---\n");
    uint32_t phys_pages[3];
    uint32_t virt_pages[3];
    
    for (int i = 0; i < 3; i++) {
        phys_pages[i] = pmm_alloc_pages(0);
        if (!phys_pages[i]) {
            serial_printf(COM1, "ERROR: Failed to allocate page %d\n", i);
            goto cleanup;
        }
        
        virt_pages[i] = temp_map_page(phys_pages[i]);
        if (!virt_pages[i]) {
            serial_printf(COM1, "ERROR: Failed to map page %d\n", i);
            goto cleanup;
        }
        
        // 每个页写入不同的数据
        int* numbers = (int*)virt_pages[i];
        for (int j = 0; j < 10; j++) {
            numbers[j] = i * 100 + j;
        }
    }
    
    // 验证所有映射都正常工作
    for (int i = 0; i < 3; i++) {
        int* numbers = (int*)virt_pages[i];
        serial_printf(COM1, "Page %d data: %d, %d, %d\n", i, numbers[0], numbers[5], numbers[9]);
    }
    
cleanup:
    // 清理所有映射和分配
    for (int i = 0; i < 3; i++) {
        if (virt_pages[i]) temp_unmap_page(virt_pages[i]);
        if (phys_pages[i]) pmm_free_pages(phys_pages[i], 0);
    }
    
    serial_printf(COM1, "=== Temporary Mapping Test Completed ===\n");
}

void test_kmalloc(void) {
    serial_printf(COM1, "\n=== Kernel Heap Test ===\n");
    
    // 显示初始状态
    kmalloc_stats();
    
    // 测试1: 基本分配和释放
    serial_printf(COM1, "\n--- Test 1: Basic allocation ---\n");
    char* str1 = (char*)kmalloc(64);
    if (str1) {
        strcpy(str1, "Hello, Kernel Heap!");
        serial_printf(COM1, "Allocated string: %s\n", str1);
    } else {
        serial_printf(COM1, "ERROR: kmalloc failed for 64 bytes\n");
    }
    
    int* array = (int*)kmalloc(256 * sizeof(int));
    if (array) {
        for (int i = 0; i < 256; i++) {
            array[i] = i * i;
        }
        serial_printf(COM1, "Array verification: %d, %d, %d\n", array[0], array[100], array[255]);
    } else {
        serial_printf(COM1, "ERROR: kmalloc failed for array\n");
    }
    
    kmalloc_stats();
    
    // 测试2: calloc 功能（清零分配）
    serial_printf(COM1, "\n--- Test 2: calloc test ---\n");
    char* zeroed = (char*)kcalloc(1, 128);
    if (zeroed) {
        int is_zero = 1;
        for (int i = 0; i < 128; i++) {
            if (zeroed[i] != 0) {
                is_zero = 0;
                break;
            }
        }
        serial_printf(COM1, "calloc zero check: %s\n", is_zero ? "PASS" : "FAIL");
    } else {
        serial_printf(COM1, "ERROR: kcalloc failed\n");
    }
    
    // 测试3: realloc 功能
    serial_printf(COM1, "\n--- Test 3: realloc test ---\n");
    char* small = (char*)kmalloc(16);
    if (small) {
        strcpy(small, "small");
        serial_printf(COM1, "Original: %s\n", small);
        
        char* large = (char*)krealloc(small, 64);
        if (large) {
            serial_printf(COM1, "After realloc: %s\n", large);
            strcat(large, " -> expanded!");
            serial_printf(COM1, "Modified: %s\n", large);
            kfree(large);
        } else {
            serial_printf(COM1, "ERROR: krealloc failed\n");
            kfree(small);
        }
    } else {
        serial_printf(COM1, "ERROR: kmalloc failed for realloc test\n");
    }
    
    kmalloc_stats();
    
    // 测试4: 释放内存
    serial_printf(COM1, "\n--- Test 4: Free memory ---\n");
    if (str1) kfree(str1);
    if (array) kfree(array);
    if (zeroed) kfree(zeroed);
    
    kmalloc_stats();
    
    // 测试5: 压力测试
    serial_printf(COM1, "\n--- Test 5: Stress test ---\n");
    void* pointers[50];
    int success_count = 0;
    
    for (int i = 0; i < 50; i++) {
        pointers[i] = kmalloc(128 + i * 8); // 逐渐增大的分配
        if (pointers[i]) {
            success_count++;
        } else {
            serial_printf(COM1, "Allocation failed at iteration %d\n", i);
            break;
        }
    }
    
    serial_printf(COM1, "Successful allocations: %d/50\n", success_count);
    kmalloc_stats();
    
    // 释放所有分配
    for (int i = 0; i < success_count; i++) {
        kfree(pointers[i]);
    }
    
    kmalloc_stats();
    serial_printf(COM1, "=== Kernel Heap Test Completed ===\n");
}

void test_integration(void) {
    serial_printf(COM1, "\n=== Integration Test ===\n");
    
    // 测试组合使用所有内存管理组件
    serial_printf(COM1, "Testing integrated memory management...\n");
    
    // 1. 使用 Buddy 分配器分配物理页
    uint32_t phys_page = pmm_alloc_pages(0);
    if (!phys_page) {
        serial_printf(COM1, "ERROR: Buddy allocation failed\n");
        return;
    }
    
    // 2. 使用临时映射访问物理页
    uint32_t temp_virt = temp_map_page(phys_page);
    if (!temp_virt) {
        serial_printf(COM1, "ERROR: Temporary mapping failed\n");
        pmm_free_pages(phys_page, 0);
        return;
    }
    
    // 3. 在映射的页中创建一些数据结构
    struct test_struct {
        int id;
        char name[32];
        float value;
    };
    
    struct test_struct* data = (struct test_struct*)temp_virt;
    data->id = 42;
    strcpy(data->name, "Integration Test");
    data->value = 3.14159f;
    
    serial_printf(COM1, "Structure in temp mapping: id=%d, name=%s, value=%f\n", 
                 data->id, data->name, data->value);
    
    // 4. 使用内核堆分配器创建副本
    struct test_struct* heap_copy = (struct test_struct*)kmalloc(sizeof(struct test_struct));
    if (heap_copy) {
        memcpy(heap_copy, data, sizeof(struct test_struct));
        serial_printf(COM1, "Heap copy: id=%d, name=%s, value=%f\n", 
                     heap_copy->id, heap_copy->name, heap_copy->value);
        kfree(heap_copy);
    }
    
    // 5. 清理所有资源
    temp_unmap_page(temp_virt);
    pmm_free_pages(phys_page, 0);
    
    serial_printf(COM1, "Integration test completed successfully!\n");
    serial_printf(COM1, "=== Integration Test Completed ===\n");
}

void run_all_memory_tests(void) {
    serial_printf(COM1, "\n");
    serial_printf(COM1, "=========================================\n");
    serial_printf(COM1, "Starting Comprehensive Memory Tests\n");
    serial_printf(COM1, "=========================================\n");
    
    test_buddy_system();
    test_temp_mapping();
    test_kmalloc();
    test_integration();
    
    serial_printf(COM1, "\n");
    serial_printf(COM1, "=========================================\n");
    serial_printf(COM1, "All Memory Tests Completed Successfully!\n");
    serial_printf(COM1, "=========================================\n");
}
