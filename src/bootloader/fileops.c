#include <efi.h>
#include <efilib.h>

#include "fileops.h"

struct {
  EFI_LOADED_IMAGE *loaded_image;
} fops_data;

void fops_init(EFI_HANDLE image_handle) {
  EFI_GUID loaded_image_protocol_guid = LOADED_IMAGE_PROTOCOL;
  EFI_STATUS status = uefi_call_wrapper(BS->HandleProtocol, 3, image_handle, &loaded_image_protocol_guid, (VOID **) &fops_data.loaded_image);

  if (EFI_ERROR(status)) {
    Print(L"fops_init() error: %d\n", status);
  }
}

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

EFI_FILE * fops_open_volume() {
  return LibOpenRoot(fops_data.loaded_image->DeviceHandle);
  /*
  EFI_FILE *file_handle;
  EFI_STATUS status = uefi_call_wrapper(fs->OpenVolume, 2, fs, &file_handle);

  if (status == EFI_SUCCESS) {
    return file_handle;
  } else {
    Print(L"fops_open_volume() error: %d\n", status);
    return NULL;
  }
  */
}

EFI_FILE * fops_open_file(EFI_FILE *root_dir, CHAR16 *filename, UINT64 mode, UINT64 attributes) {
  EFI_STATUS status = uefi_call_wrapper(root_dir->Open, 5, root_dir, &root_dir, filename, mode, attributes);

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

  EFI_STATUS status = uefi_call_wrapper(file->GetInfo, 4, file, &file_info_guid, &info_buffer_size, file_info_ptr);

  if (status == EFI_SUCCESS) {
    return file_info_ptr->FileSize;
  } else {
    Print(L"fops_file_size() error: %d\n", status);
    return -1;
  }
}

UINTN fops_file_read(EFI_FILE *file, UINTN length, OUT void *buffer) {
  EFI_STATUS status = uefi_call_wrapper(file->Read, 3, file, &length, buffer);

  if (status == EFI_SUCCESS) {
    return length;
  } else {
    Print(L"fops_file_read() error: %d\n", status);
    return 0;
  }
}

void fops_file_close(EFI_FILE *file) {
  uefi_call_wrapper(file->Close, 1, file);
}
