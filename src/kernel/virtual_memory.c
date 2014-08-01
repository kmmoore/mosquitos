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
  list free_list;

} virtual_memory;

typedef struct {
  uint64_t present:1, writable:1, user_accessable:1;
  uint64_t pwt:1, pcd:1;
  uint64_t accessed:1, dirty:1;
  uint64_t page_size:1, global:1;
  
  uint64_t reserved:3;

  uint64_t address:40;

  uint64_t ignored:11;
  uint64_t execute_disabled:1;
} PageTableEntry;

#define PTES_PER_PAGE (EFI_PAGE_SIZE / sizeof(PageTableEntry))

static void add_to_free_list(uint64_t physical_address, uint64_t num_pages) {
  // We can do this because we have identity mapping in the kernel
  // TODO: Figure out if there is a way to directly access physical memory/if this is necessary

  // Search to see if we can combine this chunk with another
  // TODO: Is this a potential DOS if the freelist gets too long?
  list_entry *current = list_head(&virtual_memory.free_list);
  list_entry *prev = NULL;
  while (current) {
    uint64_t num_pages_in_entry = list_entry_value(current);
    uint64_t physical_start = (uint64_t)current; // TODO: Make this guarantee in the list interface
    uint64_t physical_end = physical_start + num_pages_in_entry * EFI_PAGE_SIZE;

    if (physical_address >= physical_start && physical_address < physical_end) {
    } else if (physical_address == physical_end) {
      list_entry_set_value(current, num_pages_in_entry + num_pages);
      return;
    }

    prev = current;
    current = list_next(current);
  }

  // If we get here, we could not combine, insert a new chunk at the end
  // TODO: Should we keep this sorted?
  list_entry *entry = (list_entry *)physical_address;
  list_entry_set_value(entry, num_pages);
  list_insert_after(&virtual_memory.free_list, prev, entry);
}

static void setup_free_memory() {
  int num_entries = virtual_memory.mem_map_size / virtual_memory.mem_map_descriptor_size;

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

void vm_print_free_list() {
  text_output_printf("VM free list:\n");

  list_entry *current = list_head(&virtual_memory.free_list);
  while (current) {
    uint64_t num_pages = list_entry_value(current);
    text_output_printf("  Address: 0x%x, num_pages: %d\n", current, num_pages);
    current = list_next(current);
  }
}

PageTableEntry * follow_pte(PageTableEntry entry) {
  return (PageTableEntry *)(intptr_t)(entry.address << 12);
}

void vm_write_cr3(uint64_t cr3) {
  __asm__ ("movq %0, %%cr3": : "r" (cr3));
}

uint64_t vm_read_cr3() {
  uint64_t cr3;
  __asm__ ("movq %%cr3, %0": "=r" (cr3));

  return cr3;
}

void vm_init(uint8_t *memory_map, uint64_t mem_map_size, uint64_t mem_map_descriptor_size) {

  virtual_memory.memory_map = memory_map;
  virtual_memory.mem_map_size = mem_map_size;
  virtual_memory.mem_map_descriptor_size = mem_map_descriptor_size;

  virtual_memory.physical_end = 0;
  virtual_memory.num_free_pages = 0;
  list_init(&virtual_memory.free_list);

  text_output_printf("Determining free memory...");
  setup_free_memory();
  text_output_printf("Done\n");

  text_output_printf("Found %dMB of physical memory, %dMB free.\n", virtual_memory.physical_end / (1024*1024), virtual_memory.num_free_pages * 4 / 1024);

  vm_print_free_list();
}

void * vm_palloc(uint64_t num_pages) {
  list_entry *chunk = list_head(&virtual_memory.free_list);

  while (chunk) {
    if (list_entry_value(chunk) >= num_pages) break;
    chunk = list_next(chunk);
  }

  if (chunk == NULL) {
    return NULL; // We can't fulfill the request
  }

  uint64_t available_pages = list_entry_value(chunk);
  void *last_pages = ((uint8_t *)chunk) + (available_pages - num_pages) * EFI_PAGE_SIZE;

  if (available_pages == num_pages) {
    list_remove(&virtual_memory.free_list, chunk);
  } else {
    list_entry_set_value(chunk, available_pages - num_pages);
  }

  return last_pages;
}

void vm_pfree(void *physical_address, uint64_t num_pages) {
  // TODO: coalesce
  add_to_free_list((uint64_t)physical_address, num_pages);
}

void vm_map(uint64_t physical_address, void *virtual_address, uint64_t flags) {
  (void)physical_address;
  (void)virtual_address;
  (void)flags;
}

void vm_unmap(void *virtual_address) {
  (void)virtual_address;
}
