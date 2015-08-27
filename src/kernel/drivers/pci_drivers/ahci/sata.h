#include <kernel/kernel_common.h>
#include <kernel/drivers/pci_drivers/pci_device_driver.h>
#include <kernel/drivers/pci_drivers/ahci/ahci.h>
#include <kernel/drivers/pci_drivers/ahci/ahci_internal.h>

#ifndef _SATA_H
#define _SATA_H

PCIDeviceDriverError sata_fill_device_info(PCIDeviceDriver *ahci_driver, AHCIDevice *device);

PCIDeviceDriverError sata_read(PCIDeviceDriver *ahci_driver, AHCIDevice *device,
                               uint64_t address, uint32_t block_count, void *buffer,
                               uint64_t buffer_size);

PCIDeviceDriverError sata_write(PCIDeviceDriver *ahci_driver, AHCIDevice *device,
                                uint64_t address, uint32_t block_count, void *buffer,
                                uint64_t buffer_size);
#endif
