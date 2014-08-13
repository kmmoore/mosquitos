#include "../kernel_common.h"

#ifndef _VIRTUAL_MEMORY_H
#define _VIRTUAL_MEMORY_H

#define VM_PAGE_BIT_SIZE (12)
#define VM_PAGE_SIZE (1 << VM_PAGE_BIT_SIZE)

void vm_init(uint8_t *memory_map, uint64_t mem_map_size, uint64_t mem_map_descriptor_size);
void vm_print_free_list();

void * vm_palloc(uint64_t num_pages);
void * vm_pmap(uint64_t virtual_address, uint64_t num_pages);
void vm_pfree(void *virtual_address, uint64_t num_pages);

void vm_map(uint64_t physical_address, void *virtual_address, uint64_t flags);
void vm_unmap(void *virtual_address);



#endif