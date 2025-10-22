#ifndef VMM_H
#define VMM_H

#include <types.h>

uint32_t vmm_create_user_space(void);
void vmm_destroy_user_space(uint32_t page_dir);
int vmm_load_user_program(uint32_t page_dir, uint32_t binary_phys, uint32_t size);

#endif