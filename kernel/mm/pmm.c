// kernel/mm/pmm.c
#include "pmm.h"
#include "string.h"
#include "serial.h"

struct buddy_system buddy_sys;

void pmm_init(uint32_t memory_start, uint32_t memory_end) {
    serial_printf(COM1, "PMM: Initializing Buddy System...\n");
    
    // 计算总页数
    uint32_t total_memory = memory_end - memory_start;
    buddy_sys.total_pages = total_memory / PAGE_SIZE;
    buddy_sys.managed_pages = buddy_sys.total_pages;
    buddy_sys.base_addr = memory_start;
    
    serial_printf(COM1, "PMM: Memory range: 0x%x - 0x%x (%d pages)\n", 
                 memory_start, memory_end, buddy_sys.total_pages);
    
    // 初始化自旋锁
    spinlock_init(&buddy_sys.lock);
    
    // 初始化各阶空闲链表
    for (int i = 0; i <= MAX_ORDER; i++) {
        list_init(&buddy_sys.free_area[i].free_list);
        buddy_sys.free_area[i].nr_free = 0;
    }
    
    // 计算位图大小（每个页用1位表示）
    uint32_t bitmap_size = (buddy_sys.total_pages + 7) / 8;
    buddy_sys.bitmap = (uint8_t*)memory_start;
    
    // 清零位图
    memset(buddy_sys.bitmap, 0, bitmap_size);
    
    // 将所有内存作为最大块加入伙伴系统
    uint32_t max_order_pages = 1 << MAX_ORDER;
    uint32_t remaining_pages = buddy_sys.total_pages;
    uint32_t current_addr = memory_start + bitmap_size;
    
    // 对齐到最大块边界
    current_addr = (current_addr + (max_order_pages * PAGE_SIZE - 1)) & 
                   ~(max_order_pages * PAGE_SIZE - 1);
    
    while (remaining_pages >= max_order_pages) {
        struct page *page = (struct page*)current_addr;
        page->order = MAX_ORDER;
        list_add(&page->list, &buddy_sys.free_area[MAX_ORDER].free_list);
        buddy_sys.free_area[MAX_ORDER].nr_free++;
        
        // 在位图中标记这些页为已使用（管理结构占用）
        for (uint32_t i = 0; i < max_order_pages; i++) {
            uint32_t page_idx = (current_addr - memory_start) / PAGE_SIZE + i;
            buddy_sys.bitmap[page_idx / 8] |= (1 << (page_idx % 8));
        }
        
        current_addr += max_order_pages * PAGE_SIZE;
        remaining_pages -= max_order_pages;
    }
    
    serial_printf(COM1, "PMM: Buddy system initialized\n");
    pmm_stats();
}

uint32_t pmm_alloc_pages(uint32_t order) {
    if (order > MAX_ORDER) {
        serial_printf(COM1, "PMM: Invalid order %d\n", order);
        return 0;
    }
    
    spinlock_acquire(&buddy_sys.lock);
    
    // 查找合适阶数的空闲块
    uint32_t current_order = order;
    struct free_area *area;
    
    for (current_order = order; current_order <= MAX_ORDER; current_order++) {
        area = &buddy_sys.free_area[current_order];
        if (!list_empty(&area->free_list)) {
            break;
        }
    }
    
    // 没有找到空闲块
    if (current_order > MAX_ORDER) {
        spinlock_release(&buddy_sys.lock);
        serial_printf(COM1, "PMM: Out of memory for order %d\n", order);
        return 0;
    }
    
    // 从链表中取出一个块
    struct list_head *entry = area->free_list.next;
    list_del(entry);
    area->nr_free--;
    
    struct page *page = (struct page*)entry;
    uint32_t page_addr = (uint32_t)page;
    
    // 如果找到的块比需要的大，进行分裂
    while (current_order > order) {
        current_order--;
        area = &buddy_sys.free_area[current_order];
        
        // 计算伙伴块的地址
        uint32_t buddy_addr = page_addr ^ (1 << (current_order + 12)); // PAGE_SIZE = 4096 = 2^12
        
        // 创建伙伴块
        struct page *buddy = (struct page*)buddy_addr;
        buddy->order = current_order;
        list_add(&buddy->list, &area->free_list);
        area->nr_free++;
        
        // 在位图中标记伙伴块
        uint32_t buddy_idx = (buddy_addr - buddy_sys.base_addr) / PAGE_SIZE;
        buddy_sys.bitmap[buddy_idx / 8] |= (1 << (buddy_idx % 8));
    }
    
    page->order = order;
    
    // 在位图中标记分配的页
    uint32_t page_idx = (page_addr - buddy_sys.base_addr) / PAGE_SIZE;
    uint32_t pages_to_alloc = 1 << order;
    for (uint32_t i = 0; i < pages_to_alloc; i++) {
        buddy_sys.bitmap[(page_idx + i) / 8] |= (1 << ((page_idx + i) % 8));
    }
    
    spinlock_release(&buddy_sys.lock);
    
    serial_printf(COM1, "PMM: Allocated %d pages at 0x%x (order %d)\n", 
                 pages_to_alloc, page_addr, order);
    
    return page_addr;
}

void pmm_free_pages(uint32_t page_addr, uint32_t order) {
    if (order > MAX_ORDER || page_addr < buddy_sys.base_addr) {
        serial_printf(COM1, "PMM: Invalid free: addr=0x%x, order=%d\n", page_addr, order);
        return;
    }
    
    spinlock_acquire(&buddy_sys.lock);
    
    uint32_t current_order = order;
    uint32_t current_addr = page_addr;
    
    // 在位图中清除这些页
    uint32_t page_idx = (current_addr - buddy_sys.base_addr) / PAGE_SIZE;
    uint32_t pages_to_free = 1 << order;
    for (uint32_t i = 0; i < pages_to_free; i++) {
        buddy_sys.bitmap[(page_idx + i) / 8] &= ~(1 << ((page_idx + i) % 8));
    }
    
    // 尝试合并伙伴块
    while (current_order < MAX_ORDER) {
        uint32_t buddy_idx = find_buddy_pfn(page_idx, current_order);
        uint32_t buddy_addr = buddy_sys.base_addr + buddy_idx * PAGE_SIZE;
        
        // 检查伙伴块是否存在且空闲
        struct free_area *area = &buddy_sys.free_area[current_order];
        int buddy_found = 0;
        
        struct list_head *pos;
        list_for_each(pos, &area->free_list) {
            struct page *buddy_page = (struct page*)pos;
            if ((uint32_t)buddy_page == buddy_addr) {
                // 找到伙伴块，进行合并
                list_del(&buddy_page->list);
                area->nr_free--;
                buddy_found = 1;
                break;
            }
        }
        
        if (!buddy_found) {
            break;
        }
        
        // 合并块：取两个块中地址较小的那个
        if (current_addr > buddy_addr) {
            current_addr = buddy_addr;
            page_idx = buddy_idx;
        }
        
        current_order++;
    }
    
    // 将合并后的块加入相应链表
    struct page *page = (struct page*)current_addr;
    page->order = current_order;
    list_add(&page->list, &buddy_sys.free_area[current_order].free_list);
    buddy_sys.free_area[current_order].nr_free++;
    
    spinlock_release(&buddy_sys.lock);
    
    serial_printf(COM1, "PMM: Freed %d pages at 0x%x (merged to order %d)\n", 
                 pages_to_free, page_addr, current_order);
}

// 查找伙伴块的页帧号
 uint32_t find_buddy_pfn(uint32_t page_idx, uint32_t order) {
    return page_idx ^ (1 << order);
}

// 统计信息
void pmm_stats(void) {
    spinlock_acquire(&buddy_sys.lock);
    
    serial_printf(COM1, "PMM: Memory Statistics:\n");
    serial_printf(COM1, "  Total pages: %d\n", buddy_sys.total_pages);
    serial_printf(COM1, "  Managed pages: %d\n", buddy_sys.managed_pages);
    
    // 输出各阶空闲块数量
    for (int i = 0; i <= MAX_ORDER; i++) {
        uint32_t block_size = 1 << i;
        serial_printf(COM1, "  Order %d (%d pages): %d free blocks\n", 
                     i, block_size, buddy_sys.free_area[i].nr_free);
    }
    
    // 修正：基于空闲链表计算实际使用情况，而不是位图
    uint32_t total_free_pages = 0;
    for (int i = 0; i <= MAX_ORDER; i++) {
        total_free_pages += buddy_sys.free_area[i].nr_free * (1 << i);
    }
    
    uint32_t used_pages = buddy_sys.managed_pages - total_free_pages;
    uint32_t free_pages = total_free_pages;
    
    serial_printf(COM1, "  Used pages: %d\n", used_pages);
    serial_printf(COM1, "  Free pages: %d\n", free_pages);
    serial_printf(COM1, "  Usage: %d%%\n", (used_pages * 100) / buddy_sys.managed_pages);
    
    spinlock_release(&buddy_sys.lock);
}