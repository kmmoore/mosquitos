#include "../kernel_common.h"

#ifndef _AHCI_H
#define _AHCI_H

typedef struct _AHCIDevice AHCIDevice;

void ahci_init();
bool sata_read(AHCIDevice *device, uint64_t lba, uint32_t count, uint8_t *buf);

#endif
