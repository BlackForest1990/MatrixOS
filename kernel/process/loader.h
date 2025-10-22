#ifndef LOADER_H
#define LOADER_H

#include <types.h>

#define MAX_MODULES 16

typedef struct {
    char name[32];
    uint32_t start;
    uint32_t end;
    uint32_t size;
} module_info_t;

void loader_init(uint32_t mods_count, uint32_t mods_addr);
module_info_t* loader_find_module(const char* name);
uint32_t loader_get_module_count(void);
module_info_t* loader_get_module(uint32_t index);

#endif