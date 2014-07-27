#include <efi.h>
#include <efilib.h>

#include "fileops.h"

EFI_FILE_IO_INTERFACE * fops_get_filesystem() {
  EFI_FILE_IO_INTERFACE *simple_file_system;
  EFI_GUID sfs_guid = SIMPLE_FILE_SYSTEM_PROTOCOL;
   
  EFI_STATUS status = uefi_call_wrapper(BS->LocateProtocol, 3, &sfs_guid, NULL, &simple_file_system);

  if (status == EFI_SUCCESS) {
    return simple_file_system;
  } else {
    Print(L"fops_get_filesystem() error: %d\n", status);
    return NULL;
  }
}

EFI_FILE * fops_open_volume(EFI_FILE_IO_INTERFACE *fs) {
  EFI_FILE *file_handle;
  EFI_STATUS status = uefi_call_wrapper(fs->OpenVolume, 2, fs, &file_handle);

  if (status == EFI_SUCCESS) {
    return file_handle;
  } else {
    Print(L"fops_open_volume() error: %d\n", status);
    return NULL;
  }
}

EFI_FILE * fops_open_file(EFI_FILE *root_dir, CHAR16 *filename, UINT64 mode, UINT64 attributes) {
  EFI_STATUS status = uefi_call_wrapper(root_dir->Open, 5, root_dir, &root_dir,filename, mode, attributes);

  if (status == EFI_SUCCESS) {
    return root_dir;
  } else {
    Print(L"fops_open_file() error: %d\n", status);
    return NULL;
  }
}

UINTN fops_file_size(EFI_FILE *file) {
  char file_info_buffer[SIZE_OF_EFI_FILE_INFO * 2]; // We don't now how big this needs to be, hopefully this big
  EFI_FILE_INFO *file_info_ptr = (EFI_FILE_INFO *)&file_info_buffer;
  UINTN info_buffer_size = sizeof(file_info_buffer);
  EFI_GUID file_info_guid = EFI_FILE_INFO_ID;

  EFI_STATUS status = uefi_call_wr