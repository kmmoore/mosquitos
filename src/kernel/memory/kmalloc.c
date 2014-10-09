#include "kmalloc.h"
#include "virtual_memory.h"
#include "../util.h"
#include "../datastructures/list.h"
#include "../drivers/text_output.h"

typedef struct _FreeBlockHeader {
  uint64_t size:63;
  uint64_t free:1;
  list_entry entry;
} FreeBlockHeader;

typedef struct _BlockHeader {
  uint64_t size:63;
  uint64_t free:1;
  uint8_t user_data[];
} BlockHeader;

typedef struct _BlockFooter {
  uint64_t size:63;
  uint64_t free:1;
} BlockFooter;


static struct {
  list free_list;
} kmalloc_data;

void print_block(BlockHeader *header);
void print_free_list();

uint8_t *block_end(BlockHeader *header) {
  return (uint8_t *)header + sizeof(BlockHeader) + header->size + sizeof(BlockFooter);
}

BlockFooter *block_footer(BlockHeader *header) {
  return (BlockFooter *)((uint8_t *)header + sizeof(BlockHeader) + header->size);
}

BlockHeader *block_header(BlockFooter *footer) {
  return (BlockHeader *)((uint8_t *)footer - footer->size - sizeof(BlockHeader));
}

BlockHeader *previous_block_header(BlockHeader *header) {
  return block_header((BlockFooter *)header - 1);
}

BlockHeader *next_block_header(BlockHeader *header) {
  return (BlockHeader *)block_end(header);
}

FreeBlockHeader * kmalloc_increase_allocation(size_t num_bytes) {
  // Request more pages from the OS and setup sentinel regions
  // Sentinel regions keep us from trying to coalesce with memory that we don't own

  num_bytes += 2*sizeof(BlockHeader) + 2*sizeof(BlockFooter); // Add extra space for sentinel blocks
  num_bytes = ((num_bytes - 1) | VM_PAGE_SIZE) + 1; // Round up to pages

  // Amortize small requests
  if (num_bytes < kKmallocMinIncreaseBytes) num_bytes = kKmallocMinIncreaseBytes;

  uint8_t *new_chunk = vm_palloc(num_bytes / VM_PAGE_SIZE);
  if (new_chunk == NULL) return NULL;

  // Place zero-length sentinel at the beginning of the new chunk
  BlockHeader *front_sentinel_header = (BlockHeader *)new_chunk;
  front_sentinel_header->size = 0;
  front_sentinel_header->free = 0;
  BlockFooter *front_sentinel_footer = block_footer(front_sentinel_header);
  front_sentinel_footer->size = 0;
  front_sentinel_footer->free = 0;

  // Place zero-length sentinel at the end of the new chunk
  BlockHeader *back_sentinel_header = (BlockHeader *)(new_chunk + num_bytes - sizeof(BlockHeader) - sizeof(BlockFooter));
  back_sentinel_header->size = 0;
  back_sentinel_header->free = 0;
  BlockFooter *back_sentinel_footer = block_footer(back_sentinel_header);
  back_sentinel_footer->size = 0;
  back_sentinel_footer->free = 0;

  // Set up middle chunk as free, add it to the free list
  FreeBlockHeader *free_chunk = (FreeBlockHeader *)next_block_header(front_sentinel_header);
  free_chunk->size = num_bytes - 3*sizeof(BlockHeader) - 3*sizeof(BlockFooter); // Remove size of header/footer and sentinels
  free_chunk->free = 1;

  BlockFooter *footer = block_footer((BlockHeader *)free_chunk);
  footer->size = free_chunk->size;
  footer->free = 1;

  list_push_front(&kmalloc_data.free_list, &free_chunk->entry);

  return free_chunk;
}

void kmalloc_init() {
  REQUIRE_MODULE("virtual_memory");

  list_init(&kmalloc_data.free_list);
  kmalloc_increase_allocation(0); // Increase by the minimum amount

  REGISTER_MODULE("kmalloc");
}

void * kmalloc(size_t alloc_size) {
  if (alloc_size < 16) alloc_size = 16;
  alloc_size = ((alloc_size - 1) | 0xf) + 1; // Round up `alloc_size` to the nearest multiple of 16 bytes
  // TODO: I think the returned pointers have to be aligned to 64 bytes

  // Find the best fit in the free list
  list_entry *current = list_head(&kmalloc_data.free_list);
  FreeBlockHeader *chosen_header = NULL;
  while(current) {
    FreeBlockHeader *header = container_of(current, FreeBlockHeader, entry);

    if (header->size >= alloc_size) {
      if (!chosen_header || header->size < chosen_header->size) {
        chosen_header = header;
      }
    }

    current = list_next(current);
  }

  // If we couldn't find a fit, request more memory and use that
  if (!chosen_header) {
    chosen_header = kmalloc_increase_allocation(alloc_size);
    if (!chosen_header) return NULL;
  }

  // Amount of space necessary to make a new block with `alloc_size` available bytes
  size_t new_block_size = alloc_size + sizeof(BlockHeader) + sizeof(BlockFooter);

  if (chosen_header->size > new_block_size) {
    // Pull the new block off of the end of the big chunk
    BlockHeader *ret = (BlockHeader *)(block_end((BlockHeader *)chosen_header) - new_block_size);
    ret->size = alloc_size;
    ret->free = 0;

    BlockFooter *ret_footer = block_footer(ret);
    ret_footer->size = ret->size;
    ret_footer->free = 0;

    // Decrease the size of the big chunk
    chosen_header->size -= new_block_size;
    BlockFooter *new_footer = block_footer((BlockHeader *)chosen_header);
    new_footer->size = chosen_header->size;
    new_footer->free = 1;
    
    return ret->user_data;
  } else {
    // Remove full block from the free-list 
    BlockHeader *ret = (BlockHeader *)chosen_header;
    ret->free = 0;
    block_footer(ret)->free = 0;

    list_remove(&kmalloc_data.free_list, &chosen_header->entry);

    return ret->user_data;
  }
}

void kfree(void *pointer) {
  BlockHeader *header = container_of(pointer, BlockHeader, user_data);

  assert(header->size > 0);

  // Attempt to coalesce left before we add the freed block to the free list
  BlockHeader *previous_block = previous_block_header(header);

  if (previous_block->free) {
    // Coalesce with the previous block
    previous_block->size += header->size + sizeof(BlockHeader) + sizeof(BlockFooter);
    BlockFooter *previous_block_footer = block_footer(previous_block);
    previous_block_footer->size = previous_block->size;
    previous_block_footer->free = 1;

    // We are now a part of the previous block, update the pointer so we can try to coalesce right
    header = previous_block;
  } else {
    // Since we didn't coalesce left, we need to add this block to the free list
    FreeBlockHeader *free_block_header = (FreeBlockHeader *)header;
    free_block_header->free = 1;
    BlockFooter *footer = block_footer(header);
    footer->free = 1;
    list_push_front(&kmalloc_data.free_list, &free_block_header->entry);
  }

  // Attempt to coalesce right, at this point `header` is a FreeBlockHeader that is on the free list
  // (either from coalescing left, or from adding it)

  BlockHeader *next_block = next_block_header(header);

  if (next_block->free) {
    // Coalesce with the next block
    FreeBlockHeader *next_freeblock = (FreeBlockHeader *)next_block;
    list_remove(&kmalloc_data.free_list, &next_freeblock->entry);

    header->size += next_block->size + sizeof(BlockHeader) + sizeof(BlockFooter);
    BlockFooter *footer = block_footer(header);
    footer->size = header->size;
    footer->free = 1;
  }
}


void print_block(BlockHeader *header) {
  BlockFooter *footer = block_footer(header);
  text_output_printf("[%p] HSize: %lld, HFree: %d, FSize: %lld, FFree: %d\n", header, header->size, header->free, footer->size, footer->free);
  assert(header->size == footer->size);
  assert(header->free == footer->free);
}

void print_free_list() {
  text_output_printf("kmalloc() Free List:\n");

  list_entry *current = list_head(&kmalloc_data.free_list);
  while(current) {
    FreeBlockHeader *header = container_of(current, FreeBlockHeader, entry);
    print_block((BlockHeader *)header);

    current = list_next(current);
  }

  text_output_printf("\n");
}
