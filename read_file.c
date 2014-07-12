#include <efi.h>
#include <efilib.h>

EFI_STATUS
EFIAPI
efi_main (EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
  InitializeLib(ImageHandle, SystemTable);

  EFI_STATUS status;
  EFI_FILE_IO_INTERFACE *simple_file_system;
  EFI_GUID sfs_guid = SIMPLE_FILE_SYSTEM_PROTOCOL;
   
  status = uefi_call_wrapper(BS->LocateProtocol, 3, &sfs_guid, NULL, &simple_file_system);

  Print(L"Status: %d, sfs_addr: %x\n", status, simple_file_system);

  EFI_FILE *file_handle;
  status = uefi_call_wrapper(simple_file_system->OpenVolume, 2, simple_file_system, &file_handle);
  Print(L"Status: %d, fh_addr: %x\n", status, file_handle);

  status = uefi_call_wrapper(file_handle->Open, 5, file_handle, &file_handle, L"test.txt", EFI_FILE_MODE_READ, 0);

  Print(L"Status: %d, new fh addr: %x\n", status, file_handle);

  char file_info_buffer[SIZE_OF_EFI_FILE_INFO+100];
  EFI_FILE_INFO *file_info_ptr = (EFI_FILE_INFO *)&file_info_buffer;
  UINTN info_buffer_size = sizeof(file_info_buffer);
  EFI_GUID file_info_guid = EFI_FILE_INFO_ID;
  status = uefi_call_wrapper(file_handle->GetInfo, 4, file_handle, &file_info_guid, &info_buffer_size, file_info_ptr);

  UINT64 file_size = file_info_ptr->FileSize;
  Print(L"Status: %d, File size: %d\n", status, file_size);

  CHAR8 file_buffer[file_size];
  status = uefi_call_wrapper(file_handle->Read, 3, file_handle, &file_size, &file_buffer);
  Print(L"Status: %d, Contents: %s\n", status, file_buffer);

  return EFI_SUCCESS;
}