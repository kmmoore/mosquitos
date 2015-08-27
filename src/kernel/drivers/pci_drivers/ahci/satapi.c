#include <kernel/drivers/pci_drivers/ahci/satapi.h>
#include <kernel/drivers/text_output.h>
#include <common/mem_util.h>

#define SCSI_CMD_READ_12        0xA8
#define SCSI_CMD_READ_CAPACITY  0x25
#define ATAPI_CMD_PACKET        0xA0
#define ATAPI_CMD_IDENTIFY      0xA1

#define BYTES_PER_SECTOR        512

PCIDeviceDriverError satapi_scsi_command(uint8_t *command, uint8_t command_size, AHCIDevice *device,
                                         void *buffer, uint64_t buffer_size) {
  if (ahci_device_type(device) != AHCI_DEVICE_SATAPI || command_size != ATAPI_COMMAND_LENGTH) {
    return PCI_ERROR_INVALID_PARAMETERS;
  }

  int slot = ahci_begin_command(device);
  if (slot == -1) {
    return PCI_ERROR_DEVICE_ERROR;
  }

  // Setup command
  // TODO(kylemoore): Set write value properly
  FISRegisterH2D *command_fis = ahci_initialize_command_fis(device, slot, false, true, buffer_size,
                                                            buffer, true, command);
  command_fis->command = ATAPI_CMD_PACKET;

  return ahci_issue_command(device, slot) ? PCI_ERROR_NONE : PCI_ERROR_DEVICE_ERROR;
}

PCIDeviceDriverError satapi_read(PCIDeviceDriver *ahci_driver, AHCIDevice *device, uint64_t address,
                                 uint32_t block_count, void *buffer, uint64_t buffer_size) {

  (void)ahci_driver;

  struct AHCIDeviceInfo *device_info = ahci_device_info(device);
  // We can only handle 32-bit parameters (which should be plenty)
  if(address > (1l << (sizeof(uint32_t) * 8)) ||
     address + block_count > device_info->num_sectors) {
    return PCI_ERROR_INVALID_PARAMETERS;
  }

  if (buffer_size < block_count * device_info->logical_sector_size) {
    return PCI_ERROR_OUTPUT_BUFFER_TOO_SMALL;
  }

  uint8_t scsi_command[ATAPI_COMMAND_LENGTH] = { 0 };
  scsi_command[0] = SCSI_CMD_READ_12;
  for (int i = 0; i < 4; ++i) {
    scsi_command[2+i] = field_in_word(address, 3-i, 1);
  }

  for (int i = 0; i < 4; ++i) {
    scsi_command[6+i] = field_in_word(block_count, 3-i, 1);
  }

  return satapi_scsi_command(scsi_command, sizeof(scsi_command), device, buffer, buffer_size);
}

PCIDeviceDriverError satapi_write(PCIDeviceDriver *ahci_driver, AHCIDevice *device,
                                  uint64_t address, uint32_t block_count, void *buffer,
                                  uint64_t buffer_size) {
  
  (void)ahci_driver, (void)device, (void)address, (void)block_count, (void)buffer,
  (void)buffer_size;

  return PCI_ERROR_COMMAND_NOT_IMPLEMENTED;
}

// buffer should be 512 bytes
static PCIDeviceDriverError satapi_identify(PCIDeviceDriver *ahci_driver, AHCIDevice *device,
                                            void *buffer) {
  (void)ahci_driver;

  int slot = ahci_begin_command(device);
  if (slot == -1) {
    return PCI_ERROR_DEVICE_ERROR;
  }

  // Setup command
  FISRegisterH2D *command_fis = ahci_initialize_command_fis(device, slot, false, true, 512, buffer,
                                                            false, NULL);
  command_fis->command = ATAPI_CMD_IDENTIFY;
  
  return ahci_issue_command(device, slot) ? PCI_ERROR_NONE : PCI_ERROR_DEVICE_ERROR;
}

PCIDeviceDriverError satapi_capacity(AHCIDevice *device, uint64_t *num_sectors,
                                     uint32_t *sector_size) {
  uint8_t scsi_command[ATAPI_COMMAND_LENGTH] = { 0 };
  scsi_command[0] = SCSI_CMD_READ_CAPACITY;

  uint8_t buffer[8] = { 0 };

  PCIDeviceDriverError error = satapi_scsi_command(scsi_command, sizeof(scsi_command), device,
                                                   buffer, sizeof(buffer));

  if (error == PCI_ERROR_NONE) {
    *num_sectors = buffer[3] + ((uint32_t)buffer[2] << 8) + ((uint32_t)buffer[1] << 16) +
                   ((uint32_t)buffer[0] << 24) + 1;

    *sector_size = buffer[7] + ((uint32_t)buffer[6] << 8) + ((uint32_t)buffer[5] << 16) +
                   ((uint32_t)buffer[4] << 24);

  }

  return error;
}

PCIDeviceDriverError satapi_fill_device_info(PCIDeviceDriver *ahci_driver, AHCIDevice *device) {
  uint16_t buffer[256];

  PCIDeviceDriverError error = satapi_identify(ahci_driver, device, buffer);
  if (error != PCI_ERROR_NONE) {
    return error;
  }

  // Bits 15:14 should be 0b10 to indicate ATAPI
  assert((buffer[0] & (1 << 15)) && !(buffer[0] & (1 << 14)));

  struct AHCIDeviceInfo *device_info = ahci_device_info(device);

  device_info->lba48_supported = (buffer[0] & 0b11) == 1;
  ahci_copy_identify_string(buffer, 10, 10, (uint8_t *)device_info->serial_number);
  ahci_copy_identify_string(buffer, 23, 4, (uint8_t *)device_info->firmware_revision);
  ahci_copy_identify_string(buffer, 27, 20, (uint8_t *)device_info->model_number);
  ahci_copy_identify_string(buffer, 176, 60, (uint8_t *)device_info->media_serial_number);

  error = satapi_capacity(device, &device_info->num_sectors, &device_info->logical_sector_size);
  if (error != PCI_ERROR_NONE) {
    return error;
  }

  return PCI_ERROR_NONE;
}