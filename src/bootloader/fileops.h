
/*
  Wrapper functions for reading files with EFI.
  Only works while boot services are still active.
*/

#include <efi.h>
#include <efilib.h>

#ifndef _FILEOPS_H
#define _FILEOPS_H

EFI_FILE_IO_INTERFACE * fops_get_filesystem();

EFI_FILE * fops_open_volume(EFI_FILE_IO_INTERFACE *fs);

EFI_FILE * fops_open_file(EFI_FILE *root_dir, CHAR16 *filename, UINT64 mode, UINT64 attributes);

UINTN fops_file_size(EFI_FILE *file);

UINTN fops_file_read(EFI_FILE *file, UINTN length, OUT void *buffer);

void fops_file_close(EFI_FILE *file);


#endif
