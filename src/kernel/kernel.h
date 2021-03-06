#include <efi.h>
#include <efilib.h>
#include <kernel/kernel_common.h>

#ifndef _KERNEL_H_
#define _KERNEL_H_

typedef struct {
  void *xdsp_address;
  uint8_t *memory_map;
  uint64_t mem_map_size;
  uint64_t mem_map_descriptor_size;
  uint64_t kernel_lowest_address;
  uint64_t kernel_page_count;
  EFI_GRAPHICS_OUTPUT_PROTOCOL *gop;
} KernelInfo;

typedef void (*KernelMainFunc)(KernelInfo);

void kernel_main(KernelInfo info);

#endif  // _KERNEL_H_
