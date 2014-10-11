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

typedef enum {
  FIS_TYPE_REG_H2D  = 0x27, // Register FIS - host to device
  FIS_TYPE_REG_D2H  = 0x34, // Register FIS - device to host
  FIS_TYPE_DMA_ACT  = 0x39, // DMA activate FIS - device to host
  FIS_TYPE_DMA_SETUP  = 0x41, // DMA setup FIS - bidirectional
  FIS_TYPE_DATA   = 0x46, // Data FIS - bidirectional
  FIS_TYPE_BIST   = 0x58, // BIST activate FIS - bidirectional
  FIS_TYPE_PIO_SETUP  = 0x5F, // PIO setup FIS - device to host
  FIS_TYPE_DEV_BITS = 0xA1, // Set device bits FIS - device to host
} FISType;

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



typedef struct {
  // DW0
  uint8_t  cfl:5;    // Command FIS length in DWORDS, 2 ~ 16
  uint8_t  a:1;    // ATAPI
  uint8_t  w:1;    // Write, 1: H2D, 0: D2H
  uint8_t  p:1;    // Prefetchable
 
  uint8_t  r:1;    // Reset
  uint8_t  b:1;    // BIST
  uint8_t  c:1;    // Clear busy upon R_OK
  uint8_t  rsv0:1;   // Reserved
  uint8_t  pmp:4;    // Port multiplier port
 
  uint16_t  prdtl;    // Physical region descriptor table length in entries
 
  // DW1
  volatile
  uint32_t prdbc;    // Physical region descriptor byte count transferred
 
  // DW2, 3
  uint32_t ctba;   // Command table descriptor base address
  uint32_t ctbau;    // Command table descriptor base address upper 32 bits
 
  // DW4 - 7
  uint32_t rsv1[4];  // Reserved
} HBACommandHeader;

typedef struct {
  // DWORD 0
  uint8_t  fis_type; // FIS_TYPE_REG_H2D
 
  uint8_t  pmport:4; // Port multiplier
  uint8_t  rsv0:3;   // Reserved
  uint8_t  c:1;    // 1: Command, 0: Control
 
  uint8_t  command;  // Command register
  uint8_t  featurel; // Feature register, 7:0
 
  // DWORD 1
  uint8_t  lba0;   // LBA low register, 7:0
  uint8_t  lba1;   // LBA mid register, 15:8
  uint8_t  lba2;   // LBA high register, 23:16
  uint8_t  device;   // Device register
 
  // DWORD 2
  uint8_t  lba3;   // LBA register, 31:24
  uint8_t  lba4;   // LBA register, 39:32
  uint8_t  lba5;   // LBA register, 47:40
  uint8_t  featureh; // Feature register, 15:8
 
  // DWORD 3
  uint8_t  countl;   // Count register, 7:0
  uint8_t  counth;   // Count register, 15:8
  uint8_t  icc;    // Isochronous command completion
  uint8_t  control;  // Control register
 
  // DWORD 4
  uint8_t  rsv1[4];  // Reserved
} FISRegisterH2D;

typedef struct {
  // DWORD 0
  uint8_t  fis_type;    // FIS_TYPE_REG_D2H
 
  uint8_t  pmport:4;    // Port multiplier
  uint8_t  rsv0:2;      // Reserved
  uint8_t  i:1;         // Interrupt bit
  uint8_t  rsv1:1;      // Reserved
 
  uint8_t  status;      // Status register
  uint8_t  error;       // Error register
 
  // DWORD 1
  uint8_t  lba0;        // LBA low register, 7:0
  uint8_t  lba1;        // LBA mid register, 15:8
  uint8_t  lba2;        // LBA high register, 23:16
  uint8_t  device;      // Device register
 
  // DWORD 2
  uint8_t  lba3;        // LBA register, 31:24
  uint8_t  lba4;        // LBA register, 39:32
  uint8_t  lba5;        // LBA register, 47:40
  uint8_t  rsv2;        // Reserved
 
  // DWORD 3
  uint8_t  countl;      // Count register, 7:0
  uint8_t  counth;      // Count register, 15:8
  uint8_t  rsv3[2];     // Reserved
 
  // DWORD 4
  uint8_t  rsv4[4];     // Reserved
} FISRegisterD2H;

typedef struct {
  // DWORD 0
  uint8_t  fis_type; // FIS_TYPE_DATA
 
  uint8_t  pmport:4; // Port multiplier
  uint8_t  rsv0:4;   // Reserved
 
  uint8_t  rsv1[2];  // Reserved
 
  // DWORD 1 ~ N
  uint32_t data[0];  // Payload
} FISData;

typedef struct {
  // DWORD 0
  uint8_t  fis_type; // FIS_TYPE_PIO_SETUP
 
  uint8_t  pmport:4; // Port multiplier
  uint8_t  rsv0:1;   // Reserved
  uint8_t  d:1;    // Data transfer direction, 1 - device to host
  uint8_t  i:1;    // Interrupt bit
  uint8_t  rsv1:1;
 
  uint8_t  status;   // Status register
  uint8_t  error;    // Error register
 
  // DWORD 1
  uint8_t  lba0;   // LBA low register, 7:0
  uint8_t  lba1;   // LBA mid register, 15:8
  uint8_t  lba2;   // LBA high register, 23:16
  uint8_t  device;   // Device register
 
  // DWORD 2
  uint8_t  lba3;   // LBA register, 31:24
  uint8_t  lba4;   // LBA register, 39:32
  uint8_t  lba5;   // LBA register, 47:40
  uint8_t  rsv2;   // Reserved
 
  // DWORD 3
  uint8_t  countl;   // Count register, 7:0
  uint8_t  counth;   // Count register, 15:8
  uint8_t  rsv3;   // Reserved
  uint8_t  e_status; // New value of status register
 
  // DWORD 4
  uint16_t  tc;   // Transfer count
  uint8_t  rsv4[2];  // Reserved
} FISPIOSetup;

typedef struct {
  // DWORD 0
  uint8_t  fis_type; // FIS_TYPE_DMA_SETUP
 
  uint8_t  pmport:4; // Port multiplier
  uint8_t  rsv0:1;   // Reserved
  uint8_t  d:1;    // Data transfer direction, 1 - device to host
  uint8_t  i:1;    // Interrupt bit
  uint8_t  a:1;            // Auto-activate. Specifies if DMA Activate FIS is needed

  uint8_t    rsved[2];       // Reserved

  //DWORD 1&2
  uint64_t   DMAbufferID;    // DMA Buffer Identifier. Used to Identify DMA buffer in host memory. SATA Spec says host specific and not in Spec. Trying AHCI spec might work.

  //DWORD 3
  uint32_t   rsvd;           //More reserved

  //DWORD 4
  uint32_t   DMAbufOffset;   //Byte offset into buffer. First 2 bits must be 0

  //DWORD 5
  uint32_t   TransferCount;  //Number of bytes to transfer. Bit 0 must be 0

  //DWORD 6
  uint32_t   resvd;          //Reserved
 
} FISDMASetup;

 
typedef struct {
  uint32_t dba;    // Data base address
  uint32_t dbau;   // Data base address upper 32 bits
  uint32_t rsv0;   // Reserved
 
  // DW3
  uint32_t dbc:22;   // Byte count, 4M max
  uint32_t rsv1:9;   // Reserved
  uint32_t i:1;    // Interrupt on completion
} HBAPRDTEntry;

typedef struct {
  // 0x00
  uint8_t  cfis[64]; // Command FIS
 
  // 0x40
  uint8_t  acmd[16]; // ATAPI command, 12 or 16 bytes
 
  // 0x50
  uint8_t  rsv[48];  // Reserved
 
  // 0x80
  HBAPRDTEntry  prdt_entry[0];  // Physical region descriptor table entries, 0 ~ 65535
} HBACommandTable;

#endif
