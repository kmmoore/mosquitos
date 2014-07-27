#include <efi.h>
#include <efilib.h>

// Minimum number of 4kb pages allocated each time kmalloc grows the pool
#define kKMallocMinPageAllocation 10
#define kEFIPageAddressBitSize 12
#define kEFIPageByteSize (1<<kEFIPageAddressBitSize) // 4kb

struct kmalloc_block {
  struct kmalloc_block *prev, *next;
  UINT64 size:63;
  UINT64 free:1;
  char data[0];
};

static struct {
  struct kmalloc_block *free_list;
} _kmalloc_context;

static
EFI_STATUS
kmalloc_increase_allocation (IN int num_pages, OUT void **allocation_address) {
  EFI_STATUS status = uefi_call_wrapper(BS->AllocatePages, 4, AllocateAnyPages, EfiLoaderData, num_pages, allocation_address);

  if (!EFI_ERROR(status)) {
    // Put the new block on the start of the free list
    struct kmalloc_block *block = (struct kmalloc_block *)*allocation_address;

    block->prev = NULL;
    block->next = _kmalloc_context.free_list;
    block->size = num_pages * kEFIPageByteSize - sizeof(struct kmalloc_block);
    block->free = 1;

    if (_kmalloc_context.free_list) _kmalloc_context.free_list->prev = block;
    _kmalloc_context.free_list = block;
  }

  return status;
}

EFI_STATUS
kmalloc_init () {
  _kmalloc_context.free_list = NULL;

  void *base_allocation_address;

  // Request some pages from the firmware
  EFI_STATUS status = kmalloc_increase_allocation(kKMallocMinPageAllocation, &base_allocation_address);

  Print(L"[kmalloc_init] size: %d\n", _kmalloc_context.free_list->size);

  return status;
} 

EFI_STATUS
kmalloc(IN UINT64 alloc_size, OUT void **addr) {

  alloc_size = ((alloc_size - 1) | 0xf) + 1; // Round up `size` to the nearest multiple of 16 bytes

  // Find the first block that will fit the request
  struct kmalloc_block *current = _kmalloc_context.free_list;
  while (current) {
    if (current->size >= alloc_size) break;

    current = current->next;
  }

  // If there are no available blocks, request more pages
  if (current == NULL) {
    Print(L"Allocating more memory\n");

    // Request enough pages to satisfy our request, or at least the minimum number
    int num_pages = (alloc_size >> kEFIPageAddressBitSize) > kKMallocMinPageAllocation ? 
                    (alloc_size >> kEFIPageAddressBitSize) :
                    kKMallocMinPageAllocation;

    EFI_STATUS status = kmalloc_increase_allocation(num_pages, (void **)&current);
    if (EFI_ERROR(status))
      return status;
  }

  Print(L"[kmalloc] found block of size: %d, requested: %d\n", current->size, alloc_size);

  // Take new block off the end of the free block if it's too big

  struct kmalloc_block *return_block;

  if (current->size > alloc_size + sizeof(struct kmalloc_block)) {
    current->size -= alloc_size + sizeof(struct kmalloc_block);
    return_block = (struct kmalloc_block *)((char *)current + sizeof(struct kmalloc_block) + current->size);
    return_block->size = alloc_size;
  } else {
    // Remove `current` from free-list entirely
    if (current->prev) {
      current->prev->next = current->next;
    } else {
      _kmalloc_context.free_list = current->next;
    }

    return_block = current;
  }

  return_block->free = 0;
  *addr = return_block->data;

  return EFI_SUCCESS;
}

EFI_STATUS
kfree(IN void *addr) {

  // Use pointer arithmatic to get to the start of the block
  struct kmalloc_block *block = (struct kmalloc_block *)((char *)addr - sizeof(struct kmalloc_block));

  // Put the block back on the front of the free list
  // TODO: Sort the free list by size?
  // TODO: Coalesce adjacent free blocks
  block->free = 1;
  block->prev = NULL;
  block->next = _kmalloc_context.free_list;

  if (_kmalloc_context.free_list) _kmalloc_context.free_list->prev = block;
  _kmalloc_context.free_list = block;

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
efi_main (EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
  InitializeLib(ImageHandle, SystemTable);

  EFI_STATUS status;

  status = kmalloc_init();
  Print(L"kmalloc_init() return code: %d\n", status);

  void *buffer[20];

  for (int i = 0; i < 20; ++i) {
    Print(L"%d\n", i);
    status = kmalloc(4096, &buffer[i]);
    Print(L"kmalloc() return code: %d, buffer address: %x\n", status, buffer[i]);

    status = kfree(buffer[i]);
    Print(L"kfree() return code: %d\n", status);
  }

  Print(L"Walking free list...\n");
  struct kmalloc_block *current = _kmalloc_context.free_list;
  while (current) {
    Print(L"Block address: %x, Size: %d\n", current, current->size);
    current = current->next;
  }

  return EFI_SUCCESS;
}