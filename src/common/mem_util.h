#include <stdint.h>
#include <stddef.h>

#ifndef _MEM_UTIL_H
#define _MEM_UTIL_H

void * memset(void *buffer, int value, size_t length);
void * memcpy(void *destination, const void *source, size_t length);

int memcmp(const void *s1, const void *s2, size_t n);

size_t strlen(const char *s);

char * strchr(const char *s, int c);
char * strrchr(const char *s, int c);
char *strchrnul(const char *s, int c);

int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, size_t num);

size_t strlcpy(char * restrict dst, const char * restrict src, size_t size);


#endif