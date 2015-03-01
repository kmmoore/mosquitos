#include <kernel/kernel_common.h>

#ifndef _AHCI_DRIVER_H
#define _AHCI_DRIVER_H

enum AHCICommandIDs {
  AHCI_COMMAND_NUM_DEVICES,  // input_buffer is NULL, output_buffer is uint64_t *
  AHCI_COMMAND_DEVICE_INFO,  // input_buffer is struct AHCIIdentifyCommand *, output_buffer is struct AHCIDeviceInfo *
  AHCI_COMMAND_READ,         // input_buffer is struct AHCIReadCommand *
  AHCI_COMMAND_WRITE         // input_buffer is struct AHCIWriteCommand *
};

enum AHCIDeviceType {
  AHCI_DEVICE_NONE,
  AHCI_DEVICE_SATA,
  AHCI_DEVICE_SATAPI,
  AHCI_DEVICE_SEMB,
  AHCI_DEVICE_PM,
  AHCI_DEVICE_UNKNOWN
};

struct AHCIDeviceInfo {
  enum AHCIDeviceType device_type;
  char serial_number[20];
  char firmware_revision[8];
  char model_number[40];
  char media_serial_number[60];

  bool lba48_supported;
  uint32_t logical_sector_size;
  uint64_t num_sectors;
};

struct AHCIDeviceInfoCommand {
  int device_id;
};

struct AHCIReadCommand {
  int device_id;
  uint64_t address;
  uint64_t block_count;
};

struct AHCIWriteCommand {
  int device_id;
  uint64_t address;
  uint64_t block_count; 
};

void ahci_register();

#endif
