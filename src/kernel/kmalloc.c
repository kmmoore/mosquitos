#include "kmalloc.h"
#include "virtual_memory.h"
#include "datastructures/list.h"
#include "text_output.h"
#include "util.h"

// Minimum number of 4kb pages allocated each time kmalloc grows the pool
#define kKMallocMinPageAllocation 10

static struct {
  list free_list;
} kmalloc_data;

static inline uint64_t entry_size(list_entry *entry) {
  return list_entry_value(entry);
}

static inline void entry_set_size(list_entry *entry, uint64_t size) {
  list_entry_set_value(entry, size);
}

static void * kmalloc_increase_allocation (int num_pages) {
  // EFI_STATUS status = uefi_call_wrapper(BS->AllocatePages, 4, AllocateAnyPages, EfiLoaderData, num_pages, allocation_address);
  void *allocation_address = vm_palloc(num_pages);

  if (allocation_address) {
    // Put the new block on the start of the free list
    list_entry *block_entry = (list_entry *)allocation_address;
    list_entry_set_value(block_entry, num_pages * VM_PAGE_SIZE);

    list_push_front(&kmalloc_data.free_list, block_entry);
  }

  return allocation_address;
}

void kmalloc_init () {
  list_init(&kmalloc_data.free_list);

  // Request some pages from the firmware
  void *base_allocation_address = kmalloc_increase_allocation(kKMallocMinPageAllocation);

  assert(base_allocation_address != NULL);

  text_output_printf("[kmalloc_init] size: %d\n", list_entry_value((list_entry *)base_allocation_address));
} 

void * kmalloc(uint64_t alloc_size) {

  alloc_size += sizeof(list_entry); // We need space for our header
  alloc_size = ((alloc_size - 1) | 0xf) + 1; // Round up `size` to the nearest multiple of 16 bytes

  // Find the first block that will fit the request
  list_entry *current = list_head(&kmalloc_data.free_list);
  while (current) {
    if (entry_size(current) >= alloc_size) break;

    current = current->next;
  }

  // If there are no available blocks, request more pages
  if (current == NULL) {
    text_output_printf("Allocating more memory\n");

    // Request enough pages to satisfy our request, or at least the minimum number
    int num_pages = (alloc_size >> VM_PAGE_BIT_SIZE) > kKMallocMinPageAllocation ? 
                    (alloc_size >> VM_PAGE_BIT_SIZE) :
                    kKMallocMinPageAllocation;

    current = kmalloc_increase_allocation(num_pages);
    if (current == NULL) return NULL; // We couldn't increase our allocation
  }

  text_output_printf("[kmalloc] found block of size: %d, requested: %d\n", entry_size(current), alloc_size);

  // Take new block off the end of the free block if free block too big

  list_entry *return_block;
  uint64_t chosen_block_size = entry_size(current);

  if (chosen_block_size > alloc_size) {
    // Pull block off end
    entry_set_size(current, chosen_block_size - alloc_size);
    return_block = (list_entry *)((uint8_t *)current + sizeof(list_entry) + chosen_block_size - alloc_size);
    entry_set_size(return_block, alloc_size);
  } else {
    // Remove `current` from free-list entirely
    list_remove(&kmalloc_data.free_list, current);

    return_block = current;
  }

  return &return_block[1]; // Return address of data after header
}

void kfree(void *addr) {

  // Use pointer arithmatic to get to the start of the block
  list_entry *block = (list_entry *)((uint8_t *)addr - sizeof(list_entry));

  // Put the block back on the front of the free list
  // TODO: Sort the free list by size?
  // TODO: Coalesce adjacent free blocks
  list_push_front(&kmalloc_data.free_list, block);
}