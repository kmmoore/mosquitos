#include <efi.h>
#include <efilib.h>

#include "virtual_memory.h"
#include "text_output.h"
#include "util.h"
#include "datastructures/list.h"

static struct {
  uint8_t *memory_map;
  uint64_t mem_map_size;
  uint64_t mem_map_descriptor_size;

  uint64_t physical_end;
  uint64_t num_free_pages;
  list_entry *free_list;

} virtual_memory;

struct PML4E {
  uint8_t present:1, writable:1, user_accessable:1;
  uint8_t pwt:1, pcd:1;
  uint8_t accessed:1, dirty:1;
  uint8_t page_size:1, global:1;

  uint32_t reserved:4;
  uint32_t pdpte_ptr_upper:24;
};

static void add_to_free_list(uint64_t physical_address, uint64_t num_pages) {
  // We can do this because we have identity mapping in the kernel
  // TODO: Figure out if there is a way to directly access physical memory/if this is necessary

  // Search to see if we can combine this chunk with another
  // TODO: Is this a potential DOS if the freelist gets too long?
  list_entry *current = virtual_memory.free_list;
  list_entry *prev = NULL;
  while (current) {
    uint64_t physical_start = (uint64_t)current;
    uint64_t physical_end = physical_start + current->value * EFI_PAGE_SIZE;

    if (physical_address >= physical_start && physical_address < physical_end) {
    } else if (physical_address == physical_end) {
      current->value += num_pages;
      return;
    }

    prev = current;
    current = current->next;
  }

  // If we get here, we could not combine, insert a new chunk at the end
  // TODO: Should we keep this sorted?
  list_entry *entry = (list_entry *)physical_address;
  entry->value = num_pages;

  if (prev) {
    list_insert_after(prev, entry);
  } else {
    virtual_memory.free_list = entry;
  }
}

static void setup_free_memory() {
  int num_entries = virtual_memory.mem_map_size / virtual_memory.mem_map_descriptor_size;

  virtual_memory.physical_end = 0;
  virtual_memory.num_free_pages = 0;
  virtual_memory.free_list = NULL;
  for (int i = 0; i < num_entries; ++i) {
    EFI_MEMORY_DESCRIPTOR *descriptor = (EFI_MEMORY_DESCRIPTOR *)(virtual_memory.memory_map + i * virtual_memory.mem_map_descriptor_size);

    uint64_t physical_end = descriptor->PhysicalStart + descriptor->NumberOfPages * EFI_PAGE_SIZE;

    // If the type of the chunk is usable now, add it to the free list
    EFI_MEMORY_TYPE type = descriptor->Type;
    if (type == EfiLoaderCode || type == EfiBootServicesCode ||
        type == EfiBootServicesData || type == EfiConventionalMemory) { // Types of free memory after boot services are exited
      add_to_free_list(descriptor->PhysicalStart, descriptor->NumberOfPages);
      virtual_memory.num_free_pages += descriptor->NumberOfPages;
    }

    // Keep track of the highest physical address we've seen so far
    if (physical_end > virtual_memory.physical_end) virtual_memory.physical_end = physical_end;
  }
}

void vm_init(uint8_t *memory_map, uint64_t mem_map_size, uint64_t mem_map_descriptor_size) {
  uint64_t cr3;

  __asm__ ("movq %%cr3, %0": "=r" (cr3));

  text_output_printf("CR3: 0x%x\n", cr3);

  virtual_memory.memory_map = memory_map;
  virtual_memory.mem_map_size = mem_map_size;
  virtual_memory.mem_map_descriptor_size = mem_map_descriptor_size;

  text_output_printf("Determining free memory...");
  setup_free_memory();
  text_output_printf("Done\n");

  text_output_printf("Found %dMB of physical memory, %dMB free.\n", virtual_memory.physical_end / (1024*1024), virtual_memory.num_free_pages * 4 / 1024);
}

void * vm_palloc(uint64_t num_pages) {
  list_entry *chunk = virtual_memory.free_list;

  while (chunk) {
    if (chunk->value > num_pages) break;
    chunk = chunk->next;
  }

  if (chunk == NULL) {
    return NULL; // We can't fulfill the request
  }

  text_output_printf("Chunk address: 0x%x, num_pages: %d\n", chunk, num_pages);

  void *last_pages = ((uint8_t *)chunk) + (chunk->value - num_pages) * EFI_PAGE_SIZE;

  chunk->value -= num_pages;
  if (chunk->value == 0) {
    list_remove(chunk);
  }

  return last_pages;
}

void vm_pfree(void *virtual_address, uint64_t num_pages) {
  (void)virtual_address;
  (void)num_pages;
  // TODO: Fill in
}

void vm_map(uint64_t physical_address, void *virtual_address, uint64_t flags) {
  (void)physical_address;
  (void)virtual_address;
  (void)flags;
}

void vm_unmap(void *virtual_address) {
  (void)virtual_address;
}
