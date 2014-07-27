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
