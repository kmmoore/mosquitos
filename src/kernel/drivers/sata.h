#include "../kernel_common.h"

#ifndef _SATA_H
#define _SATA_H

typedef struct _AHCIDevice AHCIDevice;

void sata_init();
bool sata_read(AHCIDevice *device, uint64_t lba, uint32_t count, uint8_t *buf);

#endif
