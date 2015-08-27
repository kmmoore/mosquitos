#include <kernel/kernel_common.h>
#include <kernel/drivers/pci_drivers/pci_device_driver.h>
#include <kernel/drivers/pci_drivers/ahci/ahci.h>
#include <kernel/drivers/pci_drivers/ahci/ahci_internal.h>

#ifndef _SATAPI_H
#define _SATAPI_H

PCIDeviceDriverError satapi_fill_device_info(PCIDeviceDriver *ahci_driver, AHCIDevice *device);

PCIDeviceDriverError satapi_read(PCIDeviceDriver *ahci_driver, AHCIDevice *device,
                                 uint64_t address, uint32_t block_count, void *buffer,
                                 uint64_t buffer_size);

PCIDeviceDriverError satapi_write(PCIDeviceDriver *ahci_driver, AHCIDevice *device,
                                  uint64_t address, uint32_t block_count, void *buffer,
                                  uint64_t buffer_size);
#endif
