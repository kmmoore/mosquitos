#include <kernel/drivers/filesystem.h>
#include <kernel/drivers/filesystems/filesystem_interface.h>
#include <kernel/kernel_common.h>

#ifndef _MFS_H
#define _MFS_H

typedef struct {
  // Existing blocks of memory to back the filesystem -- will not copy. Must
  // have enough allocated
  // blocks (at least one) for the entire size of the filesystem.
  FilesystemBlock *blocks;
} MFSInMemoryInitData;

typedef struct {
  // Device id of the SATA device that contains the filesystem.
  int device_id;
} MFSSATAInitData;

void mfs_in_memory_register();
void mfs_sata_register();

#endif
