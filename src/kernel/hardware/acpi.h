#include "../kernel_common.h"

#ifndef _ACPI_H
#define _ACPI_H

typedef struct {
  char Signature[4];
  uint32_t Length;
  uint8_t Revision;
  uint8_t Checksum;
  char OEMID[6];
  char OEMTableID[8];
  uint32_t OEMRevision;
  uint32_t CreatorID;
  uint32_t CreatorRevision;
} __attribute__((packed)) ACPISDTHeader;

void acpi_init(void *xdsp_address);
ACPISDTHeader * acpi_locate_table(const char *name);
uint64_t acpi_xdsp_address();

#endif