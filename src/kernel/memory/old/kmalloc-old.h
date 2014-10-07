#include "../kernel_common.h"

#ifndef _KMALLOC_H
#define _KMALLOC_H

void kmalloc_init ();
void * kmalloc(uint64_t size);
void kfree(void *addr);

void kmalloc_print_free_list();

#endif // _KMALLOC_H