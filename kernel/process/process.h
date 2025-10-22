#ifndef PROCESS_H
#define PROCESS_H

#include <types.h>

#define MAX_PROCESSES 16
#define USER_STACK_TOP 0xBFFFFFFB
#define USER_STACK_SIZE 4096

typedef enum {
    PROCESS_UNUSED = 0,
    PROCESS_READY,
    PROCESS_RUNNING,
    PROCESS_BLOCKED,
    PROCESS_TERMINATED
} process_state_t;

typedef struct process_control_block {
    uint32_t pid;
    process_state_t state;
    char name[32];
    uint32_t page_directory;
    uint32_t entry_point;
    uint32_t user_stack;
    uint32_t kernel_stack;
    uint32_t cr3_snapshot;
} pcb_t;

void process_init(void);
uint32_t process_create_from_module(const char* module_name);
void process_start(uint32_t pid);
void process_exit_current(void);
pcb_t* process_get_current(void);
void process_list(void);

#endif