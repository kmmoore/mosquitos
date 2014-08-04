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

typedef struct {
  list_entry entry; // Must be the first entry
  uint64_t size:63;
  uint64_t free:1;
} FreeBlock;

static void add_to_free_list(FreeBlock *block) {
  block->free = 1;

  FreeBlock *prev = (FreeBlock *)list_prev(&block->entry);

  // TODO: Coalesce forward too
  if (prev && prev->free) {
    if ((uint64_t)prev + prev->size == (uint64_t)block) { 
      prev->size += block->size;
    } else {
      list_insert_after(&kmalloc_data.free_list, &prev->entry, &block->entry);
    }
  } else {
    list_push_front(&kmalloc_data.free_list, &block->entry);
  }
}

static FreeBlock * kmalloc_increase_allocation (int num_pages) {
  FreeBlock *new_block = vm_palloc(num_pages);

  if (new_block) {
    // Put the new block on the start of the free list
    new_block->size = num_pages * VM_PAGE_SIZE;

    add_to_free_list(new_block);
  }

  return new_block;
}

void kmalloc_print_free_list() {
  FreeBlock *current = (FreeBlock *)list_head(&kmalloc_data.free_list);
  text_output_printf("kmalloc() free list:\n");
  while (current) {
    text_output_printf("  0x%x, free: %d, size: %d\n", current, current->free, current->size);

    current = (FreeBlock *)list_next(&current->entry);
  }
}

void kmalloc_init () {
  list_init(&kmalloc_data.free_list);

  // Request some pages from the page allocator
  FreeBlock *initial_block = kmalloc_increase_allocation(kKMallocMinPageAllocation);

  assert(initial_block != NULL);
} 

void * kmalloc(uint64_t alloc_size) {

  alloc_size += sizeof(FreeBlock); // We need space for our header
  alloc_size = ((alloc_size - 1) | 0xf) + 1; // Round up `size` to the nearest multiple of 16 bytes

  // Find the first block that will fit the request
  FreeBlock *current = (FreeBlock *)list_head(&kmalloc_data.free_list);
  while (current) {
    if (current->size >= alloc_size) break;

    current = (FreeBlock *)list_next(&current->entry);
  }

  // If there are no available blocks, request more pages
  if (current == NULL) {
    // Request enough pages to satisfy our request, or at least the minimum number
    int num_pages = (alloc_size >> VM_PAGE_BIT_SIZE) > kKMallocMinPageAllocation ? 
                    (alloc_size >> VM_PAGE_BIT_SIZE) :
                    kKMallocMinPageAllocation;

    current = kmalloc_increase_allocation(num_pages);
    if (current == NULL) return NULL; // We couldn't increase our allocation
  }
  // Take new block off the end of the free block if free block too big

  FreeBlock *return_block;

  if (current->size > alloc_size) {
    // Pull block off end
    current->size -= alloc_size;
    return_block = (FreeBlock *)((uint8_t *)current + current->size);
    return_block->size = alloc_size;

    // TODO: Figure out a better way to do this
    list_insert_after(&kmalloc_data.free_list, &current->entry, &return_block->entry);
    list_remove(&kmalloc_data.free_list, &return_block->entry);
  } else {
    // Remove `current` from free-list entirely
    list_remove(&kmalloc_data.free_list, &current->entry);

    return_block = current;
  }

  return_block->free = 0;

  return &return_block[1]; // Return address of data after header
}

void kfree(void *addr) {

  // Use pointer arithmatic to get to the start of the block
  FreeBlock *block = (FreeBlock *)((uint8_t *)addr - sizeof(FreeBlock));

  add_to_free_list(block);
}
