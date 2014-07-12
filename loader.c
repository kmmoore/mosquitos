#include <efi.h>
#include <efilib.h>

EFI_STATUS
get_memory_map(OUT void **map, OUT UINTN *mem_map_size, OUT UINTN *map_key, OUT UINTN *descriptor_size) {
  EFI_STATUS status;
  *mem_map_size = sizeof(EFI_MEMORY_DESCRIPTOR)*48;
  UINTN mem_map_descriptor_version;

  // Keep trying larger buffers until we find one large enough to hold the map
  while (1) {
    status = uefi_call_wrapper(BS->AllocatePool, 3, EfiLoaderData, *mem_map_size, map);

    if (EFI_ERROR(status)) {
      Print(L"AllocatePool() error: %d (tried to allocate %d bytes)\n", status, *mem_map_size);
      return status;
    }

    Print(L"Trying to get map with size: %d, buffer: %x...\n", *mem_map_size, *map);
    status = uefi_call_wrapper(BS->GetMemoryMap, 5, mem_map_size, *map, map_key, descriptor_size, &mem_map_descriptor_version);

    // We can only recover from an EFI_BUFFER_TOO_SMALL error
    if (status == EFI_BUFFER_TOO_SMALL) {
      /* `mem_map_size` will point to the size needed if the previous call failed
         but we want to reserve a bit more space than we needed for the last
         call in case the new allocation changed the memory map. */

      Print(L"Too small... Needed: %d\n", *mem_map_size);

      status = uefi_call_wrapper(BS->FreePool, 1, *map);
      *mem_map_size += sizeof(EFI_MEMORY_DESCRIPTOR)*16;

      Print(L"Freed and resized\n");
    } else {
      return status; // It's up to caller to check for an error
    }
  }
}

EFI_STATUS
EFIAPI
efi_main (EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
  InitializeLib(ImageHandle, SystemTable);

  UINTN mem_map_size, mem_map_key, mem_map_descriptor_size;
  uint8_t *mem_map = NULL;

  EFI_STATUS status = get_memory_map((void **)&mem_map, &mem_map_size, &mem_map_key, &mem_map_descriptor_size);
  Print(L"get_memory_map() status: %d, buffer: %x\n", status, mem_map);

  if (EFI_ERROR(status)) {
    Print(L"Error getting memory map!\n");
    return status;
  }

  int num_map_entries = mem_map_size / mem_map_descriptor_size;

  for (int i = 0; i < num_map_entries; ++i) {
    EFI_MEMORY_DESCRIPTOR *entry = (EFI_MEMORY_DESCRIPTOR *)(mem_map + i*mem_map_descriptor_size);

    Print(L"Type: %d, PAddress: %x, VAddress: %x, Number of Pages: %d\n", entry->Type, entry->PhysicalStart, entry->VirtualStart, entry->NumberOfPages);
  }

  // Free allocation
  status = uefi_call_wrapper(BS->FreePool, 1, mem_map);

  return EFI_SUCCESS;
}