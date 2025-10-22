#include "loader.h"
#include "multiboot.h"
#include "serial.h"
#include "string.h"

static module_info_t modules[MAX_MODULES];
static uint32_t module_count = 0;

void loader_init(uint32_t mods_count, uint32_t mods_addr) {
    if (mods_count > MAX_MODULES) {
        mods_count = MAX_MODULES;
    }
    
    module_count = mods_count;
    multiboot_module_t* grub_modules = (multiboot_module_t*)mods_addr;
    
    serial_printf(COM1, "LOADER: Initializing module loader (%d modules)\n", mods_count);
    
    for (uint32_t i = 0; i < mods_count; i++) {
        modules[i].start = grub_modules[i].mod_start;
        modules[i].end = grub_modules[i].mod_end;
        modules[i].size = grub_modules[i].mod_end - grub_modules[i].mod_start;
        
        const char* grub_name = (const char*)grub_modules[i].string;
        if (grub_name) {
            strncpy(modules[i].name, grub_name, sizeof(modules[i].name) - 1);
            modules[i].name[sizeof(modules[i].name) - 1] = '\0';
        } else {
            // 使用简单的默认命名方案
            if (i == 0) strcpy(modules[i].name, "hello");
            else if (i == 1) strcpy(modules[i].name, "test"); 
            else if (i == 2) strcpy(modules[i].name, "demo");
            else strcpy(modules[i].name, "user_program");
        }
        
        serial_printf(COM1, "  Module %d: %s (0x%x - 0x%x, %d bytes)\n", 
                     i, modules[i].name, modules[i].start, modules[i].end, modules[i].size);
    }
}

module_info_t* loader_find_module(const char* name) {
    for (uint32_t i = 0; i < module_count; i++) {
        if (strcmp(modules[i].name, name) == 0) {
            return &modules[i];
        }
    }
    return NULL;
}

uint32_t loader_get_module_count(void) {
    return module_count;
}

module_info_t* loader_get_module(uint32_t index) {
    if (index >= module_count) {
        return NULL;
    }
    return &modules[index];
}