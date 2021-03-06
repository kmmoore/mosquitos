#include <kernel/kernel_common.h>

#ifndef _KMALLOC_H_
#define _KMALLOC_H_

#define kKmallocMinIncreaseBytes (32 * 4096)

void kmalloc_init();
void * kmalloc(size_t alloc_size);
void * kcalloc(size_t count, size_t size);
void kfree(void *pointer);

#endif // _KMALLOC_H_
