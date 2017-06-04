#include <kernel/drivers/filesystems/filesystem_interface.h>
#include <kernel/kernel_common.h>

#ifndef _FILESYSTEM_H
#define _FILESYSTEM_H

void filesystem_init();
void filesystem_register(Filesystem filesystem);
bool filesystem_create(const char *const identifier, Filesystem *filesystem);
const char *filesystem_error_string(FilesystemError error);

#endif
