// kernel/mm/kmalloc.c
#include "kmalloc.h"
#include "string.h"
#include "serial.h"

struct heap_manager heap_mgr;

// 对齐到8字节边界
#define ALIGN_UP(x, align) (((x) + (align) - 1) & ~((align) - 1))

// 从数据指针获取头指针
#define HEADER_FROM_DATA(ptr) ((struct kmalloc_header*)((uint8_t*)(ptr) - sizeof(struct kmalloc_header)))
// 从头指针获取数据指针
#define DATA_FROM_HEADER(header) ((void*)((uint8_t*)(header) + sizeof(struct kmalloc_header)))

void kmalloc_init(void) {
    serial_printf(COM1, "KMALLOC: Initializing kernel heap...\n");
    
    spinlock_init(&heap_mgr.lock);
    heap_mgr.head = NULL;
    heap_mgr.total_allocated = 0;
    heap_mgr.total_freed = 0;
    heap_mgr.current_usage = 0;
    
    // 分配初始堆空间（1MB）
    uint32_t initial_pages = 256; // 1MB = 256 * 4KB
    uint32_t heap_start = pmm_alloc_pages(8); // 分配 256 页 (2^8 = 256)
     
    // 初始化第一个内存块
    heap_mgr.head = (struct kmalloc_header*)heap_start;
    heap_mgr.head->size = initial_pages * PAGE_SIZE;
    heap_mgr.head->is_free = 1;
    heap_mgr.head->next = NULL;
    heap_mgr.head->prev = NULL;
    
    serial_printf(COM1, "KMALLOC: Heap initialized at 0x%x, size: %d bytes\n",
                 heap_start, heap_mgr.head->size);
}

void* kmalloc(size_t size) {
    if (size == 0) return NULL;
    if (size > KMALLOC_MAX_SIZE) {
        serial_printf(COM1, "KMALLOC: Allocation too large: %d bytes\n", size);
        return NULL;
    }
    
    spinlock_acquire(&heap_mgr.lock);
    
    // 计算实际需要的大小（包括头部和对齐）
    size_t actual_size = ALIGN_UP(size + sizeof(struct kmalloc_header), 8);
    
    // 首次适应算法查找空闲块
    struct kmalloc_header* current = heap_mgr.head;
    struct kmalloc_header* best_fit = NULL;
    
    while (current) {
        if (current->is_free && current->size >= actual_size) {
            // 使用最佳适应算法
            if (!best_fit || current->size < best_fit->size) {
                best_fit = current;
            }
        }
        current = current->next;
    }
    
    // 如果找到合适的块
    if (best_fit) {
        // 如果块远大于需要的大小，进行分裂
        if (best_fit->size >= actual_size + sizeof(struct kmalloc_header) + KMALLOC_MIN_SIZE) {
            // 分裂块
            struct kmalloc_header* new_block = (struct kmalloc_header*)((uint8_t*)best_fit + actual_size);
            new_block->size = best_fit->size - actual_size;
            new_block->is_free = 1;
            new_block->prev = best_fit;
            new_block->next = best_fit->next;
            
            if (best_fit->next) {
                best_fit->next->prev = new_block;
            }
            
            best_fit->size = actual_size;
            best_fit->next = new_block;
        }
        
        best_fit->is_free = 0;
        void* result = DATA_FROM_HEADER(best_fit);
        
        // 清零分配的内存
        memset(result, 0, size);
        
        heap_mgr.total_allocated += size;
        heap_mgr.current_usage += best_fit->size;
        
        spinlock_release(&heap_mgr.lock);
        
        serial_printf(COM1, "KMALLOC: Allocated %d bytes at 0x%x\n", size, result);
        return result;
    }
    
    // 没有找到合适的块，需要扩展堆
    // 这里简化处理，实际应该调用页分配器获取更多内存
    spinlock_release(&heap_mgr.lock);
    serial_printf(COM1, "KMALLOC: Out of memory for %d bytes\n", size);
    return NULL;
}

void kfree(void* ptr) {
    if (!ptr) return;
    
    spinlock_acquire(&heap_mgr.lock);
    
    struct kmalloc_header* header = HEADER_FROM_DATA(ptr);
    
    if (header->is_free) {
        spinlock_release(&heap_mgr.lock);
        serial_printf(COM1, "KMALLOC: Double free detected at 0x%x\n", ptr);
        return;
    }
    
    // 更新统计信息
    heap_mgr.total_freed += (header->size - sizeof(struct kmalloc_header));
    heap_mgr.current_usage -= header->size;
    
    // 标记为空闲并尝试合并相邻块
    header->is_free = 1;
    
    // 向后合并
    if (header->next && header->next->is_free) {
        header->size += header->next->size;
        header->next = header->next->next;
        if (header->next) {
            header->next->prev = header;
        }
    }
    
    // 向前合并
    if (header->prev && header->prev->is_free) {
        header->prev->size += header->size;
        header->prev->next = header->next;
        if (header->next) {
            header->next->prev = header->prev;
        }
    }
    
    spinlock_release(&heap_mgr.lock);
    
    serial_printf(COM1, "KMALLOC: Freed %d bytes at 0x%x\n", 
                 header->size - sizeof(struct kmalloc_header), ptr);
}

void* kcalloc(size_t num, size_t size) {
    size_t total_size = num * size;
    void* ptr = kmalloc(total_size);
    if (ptr) {
        memset(ptr, 0, total_size);
    }
    return ptr;
}

void* krealloc(void* ptr, size_t size) {
    if (!ptr) return kmalloc(size);
    if (size == 0) {
        kfree(ptr);
        return NULL;
    }
    
    struct kmalloc_header* header = HEADER_FROM_DATA(ptr);
    size_t old_size = header->size - sizeof(struct kmalloc_header);
    
    // 如果请求的大小小于等于当前大小，直接返回
    if (size <= old_size) {
        return ptr;
    }
    
    // 分配新内存
    void* new_ptr = kmalloc(size);
    if (!new_ptr) return NULL;
    
    // 复制数据
    memcpy(new_ptr, ptr, old_size);
    
    // 释放旧内存
    kfree(ptr);
    
    return new_ptr;
}

// 统计信息
void kmalloc_stats(void) {
    spinlock_acquire(&heap_mgr.lock);
    
    serial_printf(COM1, "=== Kernel Heap Statistics ===\n");
    serial_printf(COM1, "Total allocated: %d bytes\n", heap_mgr.total_allocated);
    serial_printf(COM1, "Total freed: %d bytes\n", heap_mgr.total_freed);
    serial_printf(COM1, "Current usage: %d bytes\n", heap_mgr.current_usage);
    
    // 遍历链表显示详细信息
    uint32_t free_blocks = 0;
    uint32_t used_blocks = 0;
    uint32_t total_free = 0;
    uint32_t total_used = 0;
    
    struct kmalloc_header* current = heap_mgr.head;
    while (current) {
        if (current->is_free) {
            free_blocks++;
            total_free += current->size;
        } else {
            used_blocks++;
            total_used += current->size;
        }
        current = current->next;
    }
    
    serial_printf(COM1, "Free blocks: %d (%d bytes)\n", free_blocks, total_free);
    serial_printf(COM1, "Used blocks: %d (%d bytes)\n", used_blocks, total_used);
    serial_printf(COM1, "Total blocks: %d\n", free_blocks + used_blocks);
    
    if (heap_mgr.current_usage > 0) {
        serial_printf(COM1, "Memory efficiency: %d%%\n", 
                     (heap_mgr.total_allocated * 100) / heap_mgr.current_usage);
    }
    
    spinlock_release(&heap_mgr.lock);
}
