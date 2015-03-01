#include <kernel/kernel_common.h>
#include <kernel/drivers/pci_drivers/pci_device_driver.h>
#include <kernel/drivers/pci_drivers/ahci/ahci_internal.h>

#ifndef _SATA_H
#define _SATA_H

PCIDeviceDriverError sata_read(PCIDeviceDriver *ahci_driver, AHCIDevice *device,
                                        uint64_t address, uint32_t block_count, uint8_t *buffer,
                                        uint64_t buffer_size);

PCIDeviceDriverError sata_identify(PCIDeviceDriver *ahci_driver,
                                            AHCIDevice *device, uint8_t *buffer,
                                            uint64_t buffer_size);

#endif
