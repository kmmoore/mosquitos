#include <efi.h>
#include <efilib.h>

// Minimum number of 4kb pages allocated each time kmalloc grows the pool
#define kKMallocMinPageAllocation 10
#define kEFIPageByteSize 4096

struct kmalloc_block {
  struct kmalloc_block *prev, *next;
  UINT64 size;
  char data[0];
};

static struct {
  struct kmalloc_block *free_list;
} _kmalloc_context;

EFI_STATUS
kmalloc_init () {
  void *base_allocation_address;

  // Request some pages from the firmware
  EFI_STATUS status = uefi_call_wrapper(BS->AllocatePages, 4, AllocateAnyPages, EfiLoaderData, kKMallocMinPageAllocation, &base_allocation_address);

  if (!EFI_ERROR(status)) {
    // Populate the free list with a single block
    struct kmalloc_block *block = (struct kmalloc_block *)base_allocation_address;

    block->prev = block->next = NULL;
    block->size = kKMallocMinPageAllocation * kEFIPageByteSize - sizeof(struct kmalloc_block);

    _kmalloc_context.free_list = block;
  }

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

  if (current == NULL) {
    // TODO: Allocate more memory if we are out
    return EFI_OUT_OF_RESOURCES;
  }

  Print(L"[kmalloc] found block of size: %d, requested: %d\n", current->size, alloc_size);

  // Take new block off the end of the free block
  current->size -= alloc_size + sizeof(struct kmalloc_block);
  
  struct kmalloc_block *return_block = (struct kmalloc_block *)((char *)current + sizeof(struct kmalloc_block) + current->size);
  return_block->size = alloc_size;
  *addr = return_block->data;

  return EFI_SUCCESS;
}

EFI_STATUS
kfree(IN void *addr) {

  // Use pointer arithmatic to get to the start of the block
  struct kmalloc_block *block = (struct kmalloc_block *)((char *)addr - sizeof(struct kmalloc_block));

  // Put the block back on the front of the free list\
  // TODO: Sort the free list by size?
  block->prev = NULL;
  block->next = _kmalloc_context.free_list;

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

  void *buffer;
  status = kmalloc(60, &buffer);
  Print(L"kmalloc() return code: %d, buffer address: %x\n", status, buffer);

  status = kfree(buffer);
  Print(L"kfree() return code: %d\n", status);

  Print(L"Walking free list...\n");
  struct kmalloc_block *current = _kmalloc_context.free_list;
  while (current) {
    Print(L"Block address: %x, Size: %d\n", current, current->size);
    current = current->next;
  }

  return EFI_SUCCESS;
}