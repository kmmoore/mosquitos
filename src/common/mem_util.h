#include <stdint.h>
#include <stddef.h>

#ifndef _MEM_UTIL_H
#define _MEM_UTIL_H

void * memset(void *buffer, int value, size_t length);
void * memcpy(void *destination, const void *source, size_t length);

int memcmp(const void *s1, const void *s2, size_t n); 

#endif