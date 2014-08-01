#include "kernel_common.h"
#include <efi.h>
#include <efilib.h>

#ifndef _KERNEL_H_
#define _KERNEL_H_

typedef struct {
  void *xdsp_address;
  uint8_t *memory_map;
  uint64_t mem_map_size;
  uint64_t mem_map_descriptor_size;
  EFI_GRAPHICS_OUTPUT_PROTOCOL *gop;
} KernelInfo;

typedef int (*KernelMainFunc) (KernelInfo);

int kernel_main(KernelInfo info);

#endif // _KERNEL_H_