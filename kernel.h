#include <efi.h>
#include <efilib.h>

#ifndef _KERNEL_H_
#define _KERNEL_H_

void kernel_main(uint8_t *memory_map, UINTN mem_map_size, UINTN mem_map_descriptor_size);

#endif // _KERNEL_H_