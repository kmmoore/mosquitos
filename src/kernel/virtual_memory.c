#include <efi.h>
#include <efilib.h>

#include "virtual_memory.h"
#include "text_output.h"
#include "util.h"

static struct {
  uint8_t *memory_map;
  uint64_t mem_map_size;
  uint64_t mem_map_descriptor_size;

} virtual_memory;

struct PML4E {
  uint8_t present:1, writable:1, user_accessable:1;
  uint8_t pwt:1, pcd:1;
  uint8_t accessed:1, dirty:1;
  uint8_t page_size:1, global:1;

  uint32_t reserved:4;
  uint32_t pdpte_ptr_upper:24;
};

static void compute_pysical_memory_properties() {
  int num_entries = virtual_memory.mem_map_size / virtual_memory.mem_map_descriptor_size;

  for (int i = 0; i < num_entries; ++i) {
    EFI_MEMORY_DESCRIPTOR *descriptor = (EFI_MEMORY_DESCRIPTOR *)(virtual_memory.memory_map + i * virtual_memory.mem_map_descriptor_size);

    text_output_printf("PhysicalStart: 0x%x, number of pages: %d\n", descriptor->PhysicalStart, descriptor->NumberOfPages);
  }
}

void vm_init(uint8_t *memory_map, uint64_t mem_map_size, uint64_t mem_map_descriptor_size) {
  uint64_t cr3;

  __asm__ ("movq %%cr3, %0": "=r" (cr3));

  text_output_printf("CR3: 0x%x\n", cr3);

  virtual_memory.memory_map = memory_map;
  virtual_memory.mem_map_size = mem_map_size;
  virtual_memory.mem_map_descriptor_size = mem_map_descriptor_size;

  compute_pysical_memory_properties();
}

void mv_map(uint64_t physical_address, uint64_t virtual_address, uint64_t flags) {
  (void)physical_address;
  (void)virtual_address;
  (void)flags;
}

void mv_unmap(uint64_t virtual_address) {
  (void)virtual_address;
}
