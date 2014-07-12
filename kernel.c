#include <efi.h>
#include <efilib.h>

#include "kernel.h"

void kernel_main(uint8_t *memory_map, UINTN mem_map_size, UINTN mem_map_descriptor_size) {
  while (1) {
    // The kernel should never return, because we have nowhere to return to
  }

  ASSERT(false); // We should never get here
}