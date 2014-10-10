#include "../kernel_common.h"

#ifndef _SATA_TYPES_H
#define _SATA_TYPES_H

typedef enum {
  AHCI_DEVICE_NONE,
  AHCI_DEVICE_SATA,
  AHCI_DEVICE_SATAPI,
  AHCI_DEVICE_SEMB,
  AHCI_DEVICE_PM,
  AHCI_DEVICE_UNKNOWN
} AHCIDeviceType;

enum {
  IPM_NOT_PRESENT  = 0x0,
  IPM_ACTIVE       = 0x1,
  IPM_PARTIAL_POER = 0x02,
  IPM_SLUMBER      = 0x06
};

enum {
  DET_NO_DEVICE        = 0x0,
  DET_NO_COMMUNICATION = 0x1,
  DET_PRESENT          = 0x3,
  DET_PHY_OFFLINE      = 0x04
};

typedef volatile struct {
  uint32_t command_list_base_address;    // 0x00, command list base address, 1K-byte aligned
  uint32_t command_list_base_address_upper;   // 0x04, command list base address upper 32 bits
  uint32_t fb;   // 0x08, FIS base address, 256-byte aligned
  uint32_t fbu;    // 0x0C, FIS base address upper 32 bits
  uint32_t interrupt_status;   // 0x10, interrupt status
  uint32_t interrupt_enable;   // 0x14, interrupt enable
  uint32_t command;    // 0x18, command and status
  uint32_t rsv0;   // 0x1C, Reserved
  uint32_t tfd;    // 0x20, task file data
  uint32_t signature;    // 0x24, signature
  uint32_t sata_status;   // 0x28, SATA status (SCR0:SStatus)
  uint32_t sata_control;   // 0x2C, SATA control (SCR2:SControl)
  uint32_t sata_error;   // 0x30, SATA error (SCR1:SError)
  uint32_t sata_active;   // 0x34, SATA active (SCR3:SActive)
  uint32_t command_issue;   // 0x38, command issue
  uint32_t sntf;   // 0x3C, SATA notification (SCR4:SNotification)
  uint32_t fbs;    // 0x40, FIS-based switch control
  uint32_t rsv1[11]; // 0x44 ~ 0x6F, Reserved
  uint32_t vendor[4];  // 0x70 ~ 0x7F, vendor specific
} __attribute__((packed)) HBAPort;

typedef volatile struct {
  // 0x00 - 0x2B, Generic Host Control
  uint32_t capabilities;    // 0x00, Host capability
  uint32_t ghc;    // 0x04, Global host control
  uint32_t interrupt_status;   // 0x08, Interrupt status
  uint32_t ports_implemented;   // 0x0C, Port implemented
  uint32_t version;   // 0x10, Version
  uint32_t ccc_ctl;  // 0x14, Command completion coalescing control
  uint32_t ccc_pts;  // 0x18, Command completion coalescing ports
  uint32_t em_loc;   // 0x1C, Enclosure management location
  uint32_t em_ctl;   // 0x20, Enclosure management control
  uint32_t extended_capabilities;   // 0x24, Host capabilities extended
  uint32_t bohc;   // 0x28, BIOS/OS handoff control and status
 
  // 0x2C - 0x9F, Reserved
  uint8_t  rsv[0xA0-0x2C];
 
  // 0xA0 - 0xFF, Vendor specific registers
  uint8_t  vendor[0x100-0xA0];
 
  // 0x100 - 0x10FF, Port control registers
  HBAPort  ports[0];
} __attribute__((packed)) HBAMemory;

#endif