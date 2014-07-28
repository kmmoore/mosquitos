#include "kernel_common.h"

#ifndef _VIRTUAL_MEMORY_H
#define _VIRTUAL_MEMORY_H

void vm_init(uint8_t *memory_map, uint64_t mem_map_size, uint64_t mem_map_descriptor_size);
void mv_map(uint64_t physical_address, uint64_t virtual_address, uint64_t flags);
void mv_unmap(uint64_t virtual_address);

#endif