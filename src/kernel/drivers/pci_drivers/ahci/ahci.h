#include <kernel/kernel_common.h>
#include <kernel/drivers/pci_drivers/ahci/ahci_types.h>
#include <kernel/drivers/pci_drivers/pci_device_driver_interface.h>

#ifndef _AHCI_H
#define _AHCI_H

typedef struct _AHCIDevice AHCIDevice;

AHCIDeviceType ahci_device_type(AHCIDevice *device);

void ahci_clear_pending_interrupts(AHCIDevice *device);
int ahci_find_command_slot(AHCIDevice *device);
FISRegisterH2D * ahci_initialize_command_fis(PCIDeviceDriverInterface *driver, AHCIDevice *device, int slot, bool write, bool prefetchable, uint64_t byte_size, uint8_t *dma_buffer);
bool ahci_issue_command(AHCIDevice *device, int slot);


// TODO: Determine if we can just call these in ahci_issue_command.
void port_start(AHCIDevice *device);
void port_stop(AHCIDevice *device);

#endif
