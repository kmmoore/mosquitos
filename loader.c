#include <efi.h>
#include <efilib.h>

#include "kernel.h"

EFI_STATUS
get_memory_map(OUT void **map, OUT UINTN *mem_map_size, OUT UINTN *mem_map_key, OUT UINTN *descriptor_size) {
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

    Print(L"Trying to get memory map with size: %d, buffer: %x...\n", *mem_map_size, *map);
    status = uefi_call_wrapper(BS->GetMemoryMap, 5, mem_map_size, *map, mem_map_key, descriptor_size, &mem_map_descriptor_version);

    // We can only recover from an EFI_BUFFER_TOO_SMALL error
    if (status == EFI_BUFFER_TOO_SMALL) {
      /*
        `mem_map_size` will point to the size needed if the previous call failed
        but we want to reserve a bit more space than we needed for the last
        call in case the new allocation changed the memory map.
      */

      status = uefi_call_wrapper(BS->FreePool, 1, *map);
      *mem_map_size += sizeof(EFI_MEMORY_DESCRIPTOR)*16;
    } else {
      return status; // It's up to caller to check for an error
    }
  }
}

int cpu_mode() {
  uint64_t mode;

  __asm__ ("movq %%cr0, %0" : "=r" (mode));

  return mode;
}

EFI_STATUS
EFIAPI
efi_main (EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
  InitializeLib(ImageHandle, SystemTable);

  Print(L"Waiting for keypress to continue booting...\n");

  UINTN event_index;
  EFI_EVENT events[1] = { SystemTable->ConIn->WaitForKey };
  uefi_call_wrapper(BS->WaitForEvent, 3, 1, events, &event_index);

  EFI_STATUS status;

  EFI_GRAPHICS_OUTPUT_PROTOCOL *gop;
  EFI_GUID gop_guid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
   
  status = uefi_call_wrapper(BS->LocateProtocol, 3, &gop_guid, NULL, &gop);

  UINTN mem_map_size = 0, mem_map_key = 0, mem_map_descriptor_size = 0;
  uint8_t *mem_map = NULL;

  // Try to exit boot services 3 times
  for (int retries = 0; retries < 3; ++retries) {
    status = get_memory_map((void **)&mem_map, &mem_map_size, &mem_map_key, &mem_map_descriptor_size);

    if (EFI_ERROR(status)) {
      Print(L"Error getting memory map!\n");
      return status;
    }

    // Exit boot services
    status = uefi_call_wrapper(BS->ExitBootServices, 2, ImageHandle, mem_map_key);
    // Execute kernel if we are successful
    if (status == EFI_SUCCESS) kernel_main(mem_map, mem_map_size, mem_map_descriptor_size, gop);
  }

  /*
    If we've reached here, we haven't managed to exit boot services,
    we should print an error, free our resources and return.
  */

  Print(L"Unable to successfully exit boot services. Last status: %d\n", status);
  uefi_call_wrapper(BS->FreePool, 1, mem_map);

  return EFI_SUCCESS;
}
