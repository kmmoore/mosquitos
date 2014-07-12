#include <efi.h>
#include <efilib.h>

#define kKMallocMinPageAllocation 10

static struct {
  int pages_allocated;
  EFI_PHYSICAL_ADDRESS page_base;
} _kmalloc_context;

EFI_STATUS
kmalloc_init () {
  EFI_STATUS status = uefi_call_wrapper(BS->AllocatePages, 4, AllocateAnyPages, EfiLoaderData, kKMallocMinPageAllocation, &_kmalloc_context.page_base);

  if (!EFI_ERROR(status)) {
    _kmalloc_context.pages_allocated = kKMallocMinPageAllocation;
  }

  Print(L"[kmalloc_init] pages allocated: %d, page_base: %x\n", _kmalloc_context.pages_allocated, _kmalloc_context.page_base);

  return status;
} 

EFI_STATUS
kmalloc(IN UINT64 size, OUT void **addr) {

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
efi_main (EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
  InitializeLib(ImageHandle, SystemTable);

  EFI_STATUS status;

  status = kmalloc_init();

  Print(L"kmalloc_init() return code: %d\n", status);

  return EFI_SUCCESS;
}