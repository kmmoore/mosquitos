#include <kernel/drivers/pci_drivers/ahci/sata.h>
#include <kernel/drivers/text_output.h>

#define ATA_CMD_READ_DMA_EX  0x25
#define ATA_CMD_WRITE_DMA_EX 0x35
#define ATA_CMD_IDENTIFY     0xEC

#define BYTES_PER_SECTOR     512

PCIDeviceDriverInterfaceError sata_read(PCIDeviceDriverInterface *ahci_driver, AHCIDevice *device,
                                        uint64_t address, uint32_t block_count, uint8_t *buffer,
                                        uint64_t buffer_size) {

  // We only know how to read from SATA devices
  assert(ahci_device_type(device) == AHCI_DEVICE_SATA);

  if (buffer_size < block_count * BYTES_PER_SECTOR) {
    return PCI_ERROR_OUTPUT_BUFFER_TOO_SMALL;
  }

  port_start(device);

  ahci_clear_pending_interrupts(device);
  int slot = ahci_find_command_slot(device);
  if (slot == -1) {
    return PCI_ERROR_DEVICE_ERROR;
  }

  uint64_t requested_bytes = block_count * BYTES_PER_SECTOR;
 
  // Setup command
  FISRegisterH2D *command_fis = ahci_initialize_command_fis(ahci_driver, device, slot, false,
                                                            true, requested_bytes, buffer);
 
  command_fis->command = ATA_CMD_READ_DMA_EX;
 
  command_fis->lba0 = field_in_word(address, 0, 1);
  command_fis->lba1 = field_in_word(address, 1, 1);
  command_fis->lba2 = field_in_word(address, 2, 1);
  command_fis->device = 1 << 6;  // LBA mode;
 
  command_fis->lba3 = field_in_word(address, 3, 1);
  command_fis->lba4 = field_in_word(address, 4, 1);
  command_fis->lba5 = field_in_word(address, 5, 1);
 
  command_fis->countl = field_in_word(block_count, 0, 1);
  command_fis->counth = field_in_word(block_count, 1, 1);
 
  bool success = ahci_issue_command(device, slot);

  // TODO: Make sure no one else is using the port
  port_stop(device);

  return success ? PCI_ERROR_NONE : PCI_ERROR_DEVICE_ERROR;
}

// buffer should be 512 bytes
PCIDeviceDriverInterfaceError sata_identify(PCIDeviceDriverInterface *ahci_driver,
                                            AHCIDevice *device, uint8_t *buffer,
                                            uint64_t buffer_size) {

  if (buffer_size < 512) {
    return PCI_ERROR_OUTPUT_BUFFER_TOO_SMALL;
  }

  port_start(device);

  ahci_clear_pending_interrupts(device);
  int slot = ahci_find_command_slot(device);
  if (slot == -1) {
    return false;
  }
 
  // Setup command
  FISRegisterH2D *command_fis = ahci_initialize_command_fis(ahci_driver, device, slot, false,
                                                            true, 512, buffer);
  command_fis->command = ATA_CMD_IDENTIFY;
  command_fis->device = 0;
 
  bool success = ahci_issue_command(device, slot);

  // TODO: Make sure no one else is using the port
  port_stop(device);

  return success ? PCI_ERROR_NONE : PCI_ERROR_DEVICE_ERROR;
}