#include <kernel/drivers/pci_drivers/ahci/sata.h>
#include <kernel/drivers/text_output.h>

#define ATA_CMD_READ_DMA_EX 0x25
#define ATA_CMD_READ_DMA 0xC8
#define ATA_CMD_WRITE_DMA_EX 0x35
#define ATA_CMD_WRITE_DMA 0xCA
#define ATA_CMD_IDENTIFY 0xEC

PCIDeviceDriverError sata_rw_command(bool write, AHCIDevice *device,
                                     uint64_t address, uint32_t block_count,
                                     void *buffer, uint64_t buffer_size) {
  if (ahci_device_type(device) != AHCI_DEVICE_SATA || block_count == 0) {
    return PCI_ERROR_INVALID_PARAMETERS;
  }

  const struct AHCIDeviceInfo *device_info = ahci_device_info(device);

  if (buffer_size < block_count * device_info->logical_sector_size) {
    return PCI_ERROR_OUTPUT_BUFFER_TOO_SMALL;
  }

  const int slot = ahci_begin_command(device);
  if (slot == -1) {
    return PCI_ERROR_DEVICE_ERROR;
  }

  // Setup command
  const uint64_t requested_bytes =
      block_count * device_info->logical_sector_size;
  FISRegisterH2D *command_fis = ahci_initialize_command_fis(
      device, slot, write, true, requested_bytes, buffer, false, NULL);

  if (device_info->lba48_supported) {
    command_fis->command = write ? ATA_CMD_WRITE_DMA_EX : ATA_CMD_READ_DMA_EX;
  } else {
    command_fis->command = write ? ATA_CMD_WRITE_DMA : ATA_CMD_READ_DMA;
  }

  ahci_set_command_fis_lba(command_fis, address, block_count);

  return ahci_issue_command(device, slot) ? PCI_ERROR_NONE
                                          : PCI_ERROR_DEVICE_ERROR;
}

PCIDeviceDriverError sata_read(PCIDeviceDriver *ahci_driver, AHCIDevice *device,
                               uint64_t address, uint32_t block_count,
                               void *buffer, uint64_t buffer_size) {
  (void)ahci_driver;
  return sata_rw_command(false, device, address, block_count, buffer,
                         buffer_size);
}

PCIDeviceDriverError sata_write(PCIDeviceDriver *ahci_driver,
                                AHCIDevice *device, uint64_t address,
                                uint32_t block_count, void *buffer,
                                uint64_t buffer_size) {
  (void)ahci_driver;
  return sata_rw_command(true, device, address, block_count, buffer,
                         buffer_size);
}

// buffer should be 512 bytes
static PCIDeviceDriverError sata_identify(PCIDeviceDriver *ahci_driver,
                                          AHCIDevice *device, void *buffer) {
  (void)ahci_driver;

  int slot = ahci_begin_command(device);
  if (slot == -1) {
    return PCI_ERROR_DEVICE_ERROR;
  }

  // Setup command
  FISRegisterH2D *command_fis = ahci_initialize_command_fis(
      device, slot, false, true, 512, buffer, false, NULL);
  command_fis->command = ATA_CMD_IDENTIFY;
  command_fis->device = 0;

  return ahci_issue_command(device, slot) ? PCI_ERROR_NONE
                                          : PCI_ERROR_DEVICE_ERROR;
}

PCIDeviceDriverError sata_fill_device_info(PCIDeviceDriver *ahci_driver,
                                           AHCIDevice *device) {
  uint16_t buffer[256];

  PCIDeviceDriverError error = sata_identify(ahci_driver, device, buffer);
  if (error != PCI_ERROR_NONE) {
    return error;
  }

  struct AHCIDeviceInfo *device_info = ahci_device_info(device);
  ahci_copy_identify_string(buffer, 10, 10,
                            (uint8_t *)device_info->serial_number);
  ahci_copy_identify_string(buffer, 23, 4,
                            (uint8_t *)device_info->firmware_revision);
  ahci_copy_identify_string(buffer, 27, 20,
                            (uint8_t *)device_info->model_number);
  ahci_copy_identify_string(buffer, 176, 60,
                            (uint8_t *)device_info->media_serial_number);

  uint16_t sector_size_info = buffer[106];
  if ((sector_size_info & (1 << 14)) != 0 &&  // Must be 1
      (sector_size_info & (1 << 15)) == 0 &&  // Must be 0
      (sector_size_info & (1 << 12)) != 0) {  // Logical sector size supported
    // Read value is in words, not bytes
    device_info->logical_sector_size =
        (buffer[117] + ((uint64_t)buffer[118] << 16)) / 2;
  } else {
    device_info->logical_sector_size = 512;
  }

  device_info->lba48_supported = (buffer[83] & (1 << 10)) != 0;

  if (device_info->lba48_supported) {
    device_info->num_sectors = buffer[100] + ((uint64_t)buffer[101] << 16) +
                               ((uint64_t)buffer[102] << 32) +
                               ((uint64_t)buffer[103] << 48);
  } else {
    device_info->num_sectors = (buffer[60] + ((uint64_t)buffer[61] << 16));
  }

  return PCI_ERROR_NONE;
}
