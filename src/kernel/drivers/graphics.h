#include <kernel/kernel_common.h>

#include <efi.h>
#include <efilib.h>

#ifndef _GRAPHICS_H
#define _GRAPHICS_H

void graphics_init(EFI_GRAPHICS_OUTPUT_PROTOCOL *gop);

#endif
