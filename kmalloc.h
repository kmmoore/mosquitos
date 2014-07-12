#ifndef _KMALLOC_H
#define _KMALLOC_H

#include <efi.h>
#include <efilib.h>

EFI_STATUS kmalloc_init ();
EFI_STATUS kmalloc(IN UINT64 size, OUT void **addr);

#endif // _KMALLOC_H