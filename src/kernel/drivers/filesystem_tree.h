#include <kernel/drivers/filesystem.h>
#include <kernel/drivers/filesystems/filesystem_interface.h>
#include <kernel/drivers/pci.h>
#include <kernel/drivers/pci_drivers/ahci/ahci.h>
#include <kernel/kernel_common.h>

#ifndef _FILESYSTEM_TREE_H
#define _FILESYSTEM_TREE_H

void filesystem_tree_init();
FilesystemError filesystem_tree_add(const char *const identifier,
                                    const void *initialization_data,
                                    Filesystem **filesystem);
Filesystem *filesystem_tree_get(size_t index);
size_t filesystem_tree_size();

#endif
