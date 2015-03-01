#include <kernel/kernel_common.h>
#include <kernel/drivers/pci_drivers/pci_device_driver_interface.h>
#include <kernel/drivers/pci_drivers/ahci/ahci.h>

#ifndef _SATA_H
#define _SATA_H

PCIDeviceDriverInterfaceError sata_read(PCIDeviceDriverInterface *ahci_driver, AHCIDevice *device,
                                        uint64_t address, uint32_t block_count, uint8_t *buffer,
                                        uint64_t buffer_size);

PCIDeviceDriverInterfaceError sata_identify(PCIDeviceDriverInterface *ahci_driver,
                                            AHCIDevice *device, uint8_t *buffer,
                                            uint64_t buffer_size);

#endif
