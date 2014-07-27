#include "kernel_common.h"
#include <efi.h>
#include <efilib.h>

#ifndef _KERNEL_H_
#define _KERNEL_H_

#define KERNEL_MAIN_FUNC_SIG int (*) (uint8_t *, uint64_t, uint64_t, EFI_GRAPHICS_OUTPUT_PROTOCOL *)

int kernel_main(uint8_t *memory_map, uint64_t mem_map_size, uint64_t mem_map_descriptor_size, EFI_GRAPHICS_OUTPUT_PROTOCOL *gop);

#endif // _KERNEL_H_