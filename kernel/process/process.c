#include "process.h"
#include "loader.h"
#include "vmm.h"
#include "pmm.h"
#include "serial.h"
#include "string.h"
#include "temp_mapping.h"

static pcb_t processes[MAX_PROCESSES];
static pcb_t* current_process = NULL;
static uint32_t next_pid = 1;

void process_init(void) {
    serial_printf(COM1, "PROCESS: Initializing process manager\n");
    memset(processes, 0, sizeof(processes));
}

uint32_t process_create_from_module(const char* module_name) {
    module_info_t* module = loader_find_module(module_name);
    if (!module) {
        serial_printf(COM1, "PROCESS: Module '%s' not found\n", module_name);
        return 0;
    }
    
    pcb_t* pcb = NULL;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (processes[i].state == PROCESS_UNUSED) {
            pcb = &processes[i];
            break;
        }
    }
    
    if (!pcb) {
        serial_printf(COM1, "PROCESS: No free process slots\n");
        return 0;
    }
    
    uint32_t user_page_dir = vmm_create_user_space();
    if (!user_page_dir) {
        serial_printf(COM1, "PROCESS: Failed to create user address space\n");
        return 0;
    }
    
    if (!vmm_load_user_program(user_page_dir, module->start, module->size)) {
        serial_printf(COM1, "PROCESS: Failed to load user program\n");
        vmm_destroy_user_space(user_page_dir);
        return 0;
    }
    
    uint32_t kernel_stack = pmm_alloc_pages(0);
    if (!kernel_stack) {
        serial_printf(COM1, "PROCESS: Failed to allocate kernel stack\n");
        vmm_destroy_user_space(user_page_dir);
        return 0;
    }
    
    pcb->pid = next_pid++;
    pcb->state = PROCESS_READY;
    strncpy(pcb->name, module_name, sizeof(pcb->name) - 1);
    pcb->page_directory = user_page_dir;
    pcb->entry_point = 0x00000000;
    pcb->user_stack = USER_STACK_TOP;
    pcb->kernel_stack = kernel_stack + PAGE_SIZE;
    
    serial_printf(COM1, "PROCESS: Created process %d '%s'\n", pcb->pid, pcb->name);
    serial_printf(COM1, "  Page dir: 0x%x, Size: %d bytes\n", user_page_dir, module->size);

    // 详细的页表映射验证
    serial_printf(COM1, "PROCESS: Detailed page table verification:\n");
    
    uint32_t temp_page_dir = temp_map_page(user_page_dir);
    uint32_t* page_dir = (uint32_t*)temp_page_dir;
    
    // 检查页目录项0（虚拟地址 0x00000000 - 0x003FFFFF）
    serial_printf(COM1, "  Page directory[0] = 0x%x\n", page_dir[0]);
    
    if (page_dir[0] & 0x1) {
        uint32_t page_table_phys = page_dir[0] & 0xFFFFF000;
        serial_printf(COM1, "  Page table physical: 0x%x\n", page_table_phys);
        
        uint32_t temp_table = temp_map_page(page_table_phys);
        uint32_t* page_table = (uint32_t*)temp_table;
        
        // 检查第一个页表项（虚拟地址 0x00000000）
        serial_printf(COM1, "  Page table[0] = 0x%x\n", page_table[0]);
        
        if (page_table[0] & 0x1) {
            uint32_t user_page_phys = page_table[0] & 0xFFFFF000;
            serial_printf(COM1, "  User page physical: 0x%x\n", user_page_phys);
            serial_printf(COM1, "  Expected: 0x2101000, Got: 0x%x\n", user_page_phys);
            
            if (user_page_phys == 0x2101000) {
                serial_printf(COM1, "  ✅ Page mapping CORRECT\n");
                
                // 验证用户代码页内容
                uint32_t temp_user_page = temp_map_page(user_page_phys);
                uint8_t* user_code = (uint8_t*)temp_user_page;
                
                serial_printf(COM1, "  User code at 0x00000000: ");
                for (int i = 0; i < 16; i++) {
                    serial_printf(COM1, "%x ", user_code[i]);
                }
                serial_printf(COM1, "\n");
                
                // 反汇编前几条指令
                serial_printf(COM1, "  Disassembly:\n");
                // mov ax, 0x23
                if (user_code[0] == 0xB8 && user_code[1] == 0x23) {
                    serial_printf(COM1, "    mov ax, 0x23\n");
                }
                // mov ds, ax  
                if (user_code[3] == 0x8E && user_code[4] == 0xD8) {
                    serial_printf(COM1, "    mov ds, ax\n");
                }
                
                temp_unmap_page(temp_user_page);
            } else {
                serial_printf(COM1, "  ❌ Page mapping WRONG!\n");
            }
        } else {
            serial_printf(COM1, "  ❌ Page table entry not present!\n");
        }
        
        temp_unmap_page(temp_table);
    } else {
        serial_printf(COM1, "  ❌ Page directory entry not present!\n");
    }
    
    temp_unmap_page(temp_page_dir);
    
    return pcb->pid;
}

void process_start(uint32_t pid) {
    pcb_t* pcb = NULL;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (processes[i].pid == pid && processes[i].state == PROCESS_READY) {
            pcb = &processes[i];
            break;
        }
    }
    
    if (!pcb) {
        serial_printf(COM1, "PROCESS: Cannot start invalid process %d\n", pid);
        return;
    }
    
    current_process = pcb;
    pcb->state = PROCESS_RUNNING;
    
    serial_printf(COM1, "PROCESS: Starting user process %d '%s'...\n", pid, pcb->name);
    
    extern void switch_to_user_mode(uint32_t page_dir, uint32_t entry, uint32_t stack);
    switch_to_user_mode(pcb->page_directory, pcb->entry_point, pcb->user_stack);
}

void process_exit_current(void) {
    if (!current_process) return;
    
    serial_printf(COM1, "PROCESS: Process %d '%s' exiting\n", 
                 current_process->pid, current_process->name);
    
    vmm_destroy_user_space(current_process->page_directory);
    pmm_free_pages(current_process->kernel_stack - PAGE_SIZE, 0);
    
    current_process->state = PROCESS_TERMINATED;
    current_process = NULL;
}

pcb_t* process_get_current(void) {
    return current_process;
}

void process_list(void) {
    serial_printf(COM1, "PROCESS: Process list:\n");
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (processes[i].state != PROCESS_UNUSED) {
            const char* state_str = "UNKNOWN";
            switch (processes[i].state) {
                case PROCESS_READY: state_str = "READY"; break;
                case PROCESS_RUNNING: state_str = "RUNNING"; break;
                case PROCESS_TERMINATED: state_str = "TERMINATED"; break;
                case PROCESS_BLOCKED: state_str = "BLOCKED"; break;
                default:break;
            }
            serial_printf(COM1, "  PID %d: %s [%s]\n", 
                         processes[i].pid, processes[i].name, state_str);
        }
    }
}