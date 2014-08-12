#include <efi.h>
#include <efilib.h>

#include "virtual_memory.h"
#include "../util.h"

#include "../drivers/text_output.h"
#include "../datastructures/list.h"
#include "../memory/kmalloc.h"

static struct {
  uint8_t *memory_map;
  uint64_t mem_map_size;
  uint64_t mem_map_descriptor_size;

  uint64_t physical_end;
  uint64_t num_free_pages;
  list free_list;

} virtual_memory_data;

typedef struct {
  list_entry entry;
  uint64_t num_pages;
} FreeBlock;

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

static inline uint64_t physical_start(FreeBlock *block) {
  return (uint64_t)block;
}

static inline uint64_t physical_end(FreeBlock *block) {
  return physical_start(block) + block->num_pages * VM_PAGE_SIZE;
}

static void add_to_free_list(uint64_t physical_address, uint64_t num_pages) {
  // We can do this because we have identity mapping in the kernel
  // TODO: Figure out if there is a way to directly access physical memory/if this is necessary

  // Search to determine where this block should be inserted
  FreeBlock *current = (FreeBlock *)list_head(&virtual_memory_data.free_list);
  while (current) {
    if (physical_address <= physical_end(current)) break;

    current = (FreeBlock *)list_next(&current->entry);
  }

  if (current && physical_address == physical_end(current)) {
    // We can combine
    current->num_pages += num_pages;
  } else {
    // Insert new chunk 
    FreeBlock *new_block = (FreeBlock *)physical_address;
    new_block->num_pages = num_pages;

    if (current) {
      list_insert_before(&virtual_memory_data.free_list, &current->entry, &new_block->entry);
    } else {
      // We couldn't find a block to insert this before
      list_push_back(&virtual_memory_data.free_list, &new_block->entry);
    }

    current = new_block;
  }

  // Coalesce if possible (current is now the block we created/added to)
  FreeBlock *next_block = (FreeBlock *)list_next(&current->entry);
  if (next_block && physical_end(current) == physical_start(next_block)) {
    list_remove(&virtual_memory_data.free_list, &next_block->entry);
    current->num_pages += next_block->num_pages;
  }
}

static void setup_free_memory() {
  int num_entries = virtual_memory_data.mem_map_size / virtual_memory_data.mem_map_descriptor_size;

  for (int i = 0; i < num_entries; ++i) {
    EFI_MEMORY_DESCRIPTOR *descriptor = (EFI_MEMORY_DESCRIPTOR *)(virtual_memory_data.memory_map + i * virtual_memory_data.mem_map_descriptor_size);

    uint64_t physical_end = descriptor->PhysicalStart + descriptor->NumberOfPages * EFI_PAGE_SIZE;

    // If the type of the chunk is usable now, add it to the free list
    EFI_MEMORY_TYPE type = descriptor->Type;
    if (type == EfiLoaderCode || type == EfiBootServicesCode ||
        type == EfiBootServicesData || type == EfiConventionalMemory) { // Types of free memory after boot services are exited
      if (descriptor->PhysicalStart < 0x100000) {
        continue; // Low memory is not actually available
        // TODO: Fix this, some of this memory (i.e., page 1) is used for things that we don't know about
      }
      add_to_free_list(descriptor->PhysicalStart, descriptor->NumberOfPages);
      virtual_memory_data.num_free_pages += descriptor->NumberOfPages;
    }

    // Keep track of the highest physical address we've seen so far
    if (physical_end > virtual_memory_data.physical_end) virtual_memory_data.physical_end = physical_end;
  }
}

void vm_print_free_list() {
  text_output_printf("VM free list:\n");

  FreeBlock *current = (FreeBlock *)list_head(&virtual_memory_data.free_list);
  while (current) {
    text_output_printf("  Address: 0x%x, num_pages: %d\n", current, current->num_pages);
    current = (FreeBlock *)list_next(&current->entry);
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

  virtual_memory_data.memory_map = memory_map;
  virtual_memory_data.mem_map_size = mem_map_size;
  virtual_memory_data.mem_map_descriptor_size = mem_map_descriptor_size;

  virtual_memory_data.physical_end = 0;
  virtual_memory_data.num_free_pages = 0;
  list_init(&virtual_memory_data.free_list);

  text_output_printf("Determining free memory...");
  setup_free_memory();
  text_output_printf("Done\n");

  text_output_printf("Found %dMB of physical memory, %dMB free.\n", virtual_memory_data.physical_end / (1024*1024), virtual_memory_data.num_free_pages * 4 / 1024);

  vm_print_free_list();

  kmalloc_init();
}

void * vm_palloc(uint64_t num_pages) {
  FreeBlock *chunk = (FreeBlock *)list_head(&virtual_memory_data.free_list);

  while (chunk) {
    if (chunk->num_pages >= num_pages) break;
    chunk = (FreeBlock *)list_next(&chunk->entry);
  }

  if (chunk == NULL) {
    return NULL; // We can't fulfill the request
  }

  void *last_pages = ((uint8_t *)chunk) + (chunk->num_pages - num_pages) * VM_PAGE_SIZE;

  if (chunk->num_pages == num_pages) {
    list_remove(&virtual_memory_data.free_list, &chunk->entry);
  } else {
    chunk->num_pages -= num_pages;
  }

  return last_pages;
}

void vm_pfree(void *physical_address, uint64_t num_pages) {
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