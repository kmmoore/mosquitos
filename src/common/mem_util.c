#include <stddef.h>
#include <stdint.h>

#include "mem_util.h"

void * memset(void *buffer, int value, size_t length) {
  for (size_t i = 0; i < length; ++i) {
    ((uint8_t *)buffer)[i] = (uint8_t)value;
  }
  return buffer;
}

void * memcpy(void *destination, const void *source, size_t length) {
  for (size_t i = 0; i < length; ++i) {
    ((uint8_t *)destination)[i] = ((uint8_t *)source)[i];
  }
  return destination;
}

int memcmp(const void *s1, const void *s2, size_t n) {
  const uint8_t *a = (const uint8_t *)s1, *b = (const uint8_t *)s2;
  
  for (size_t i = 0; i < n; ++i) {
    if (a[i] != b[i]) {
      return a[i] - b[i];
    }
  }

  return 0;
}

size_t strlen(const char *s) {
  size_t length = 0;
  while (s[length] != 0) length++;

  return length;
}

int strcmp(const char *s1, const char *s2) {
  while (*s1 == *s2 && *s1 != '\0' && *s2 != '\0') {
    ++s1;
    ++s2;
  }

  return *s1 - *s2;
}

size_t strlcpy(char * restrict dst, const char * restrict src, size_t size) {
  size_t chars_read = 0;
  while (src[chars_read] != '\0') {
    if (chars_read < size - 1) {
      dst[chars_read] = src[chars_read];
    }
    ++chars_read;
  }

  dst[size-1] = '\0';

  return chars_read;
}

