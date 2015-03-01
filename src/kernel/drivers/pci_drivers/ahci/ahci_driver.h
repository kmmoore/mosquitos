#include <kernel/kernel_common.h>

#ifndef _AHCI_DRIVER_H
#define _AHCI_DRIVER_H

enum AHCICommandIDs {
  AHCI_COMMAND_LIST_DEVICES, // input_buffer is NULL
  AHCI_COMMAND_IDENTIFY,     // input_buffer is struct AHCIIdentifyCommand *
  AHCI_COMMAND_READ,         // input_buffer is struct AHCIReadCommand *
  AHCI_COMMAND_WRITE         // input_buffer is struct AHCIWriteCommand *
};

struct AHCIIdentifyCommand {
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
