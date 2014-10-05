#include "acpi.h"
#include "../util.h"
#include "../drivers/text_output.h"
#include "../../common/mem_util.h"

typedef struct {
  uint8_t signature[8];   /* ``RSD PTR '' */
  uint8_t checksum;
  uint8_t oem_id[6];
  uint8_t revision;
  uint32_t rsdt_phys;
  uint32_t rsdt_len;
  uint64_t xsdt_phys;
  uint8_t  xchecksum;
  uint8_t __reserved[3];
} __attribute__((packed)) XDSP;


// typedef struct {
//   uint8_t AddressSpace;
//   uint8_t BitWidth;
//   uint8_t BitOffset;
//   uint8_t AccessSize;
//   uint64_t Address;
// } GenericAddressStructure;

// typedef struct {
//     ACPISDTHeader h;
//     uint32_t FirmwareCtrl;
//     uint32_t Dsdt;
 
//     // field used in ACPI 1.0; no longer in use, for compatibility only
//     uint8_t  Reserved;
 
//     uint8_t  PreferredPowerManagementProfile;
//     uint16_t SCI_Interrupt;
//     uint32_t SMI_CommandPort;
//     uint8_t  AcpiEnable;
//     uint8_t  AcpiDisable;
//     uint8_t  S4BIOS_REQ;
//     uint8_t  PSTATE_Control;
//     uint32_t PM1aEventBlock;
//     uint32_t PM1bEventBlock;
//     uint32_t PM1aControlBlock;
//     uint32_t PM1bControlBlock;
//     uint32_t PM2ControlBlock;
//     uint32_t PMTimerBlock;
//     uint32_t GPE0Block;
//     uint32_t GPE1Block;
//     uint8_t  PM1EventLength;
//     uint8_t  PM1ControlLength;
//     uint8_t  PM2ControlLength;
//     uint8_t  PMTimerLength;
//     uint8_t  GPE0Length;
//     uint8_t  GPE1Length;
//     uint8_t  GPE1Base;
//     uint8_t  CStateControl;
//     uint16_t WorstC2Latency;
//     uint16_t WorstC3Latency;
//     uint16_t FlushSize;
//     uint16_t FlushStride;
//     uint8_t  DutyOffset;
//     uint8_t  DutyWidth;
//     uint8_t  DayAlarm;
//     uint8_t  MonthAlarm;
//     uint8_t  Century;
 
//     // reserved in ACPI 1.0; used since ACPI 2.0+
//     uint16_t BootArchitectureFlags;
 
//     uint8_t  Reserved2;
//     uint32_t Flags;
 
//     // 12 byte structure; see below for details
//     GenericAddressStructure ResetReg;
 
//     uint8_t  ResetValue;
//     uint8_t  Reserved3[3];
 
//     // 64bit pointers - Available on ACPI 2.0+
//     uint64_t                X_FirmwareControl;
//     uint64_t                X_Dsdt;
 
//     GenericAddressStructure X_PM1aEventBlock;
//     GenericAddressStructure X_PM1bEventBlock;
//     GenericAddressStructure X_PM1aControlBlock;
//     GenericAddressStructure X_PM1bControlBlock;
//     GenericAddressStructure X_PM2ControlBlock;
//     GenericAddressStructure X_PMTimerBlock;
//     GenericAddressStructure X_GPE0Block;
//     GenericAddressStructure X_GPE1Block;
// } __attribute__((packed)) FADT;

typedef struct {
  ACPISDTHeader header;
  ACPISDTHeader *table_pointers;
} __attribute__((packed)) XSDT;

static struct {
  uint64_t xdsp_address;
  XSDT *xsdt;
} acpi;

bool validate_acpi_checksum(void *buf, size_t length) {
  uint8_t test = 0;
  for (size_t i = 0; i < length; ++i) {
    test += ((uint8_t *)buf)[i];
  }

  return test == 0;
}

bool validate_xdsp(XDSP *xdsp) {
  static uint8_t signature[] = {'R', 'S', 'D', ' ', 'P', 'T', 'R', ' '};
  if (memcmp(xdsp->signature, signature, sizeof(signature)) != 0) {
    return false;
  }

  return validate_acpi_checksum(xdsp, sizeof(XDSP));
}

ACPISDTHeader * acpi_locate_table(const char *name) {
  int entries = (acpi.xsdt->header.Length - sizeof(acpi.xsdt->header)) / sizeof(ACPISDTHeader *);

  for (int i = 0; i < entries; i++) {
    // TODO: Figure out why this works
    ACPISDTHeader *header = (&acpi.xsdt->table_pointers)[i];

    if (memcmp(header->Signature, name, sizeof(header->Signature)) == 0) {
      if (validate_acpi_checksum(header, header->Length)) {
        return header;
      }
    }
  }

  return NULL;
}

uint64_t acpi_xdsp_address() {
  return acpi.xdsp_address;
}

void acpi_init(void *xdsp_address) {
  acpi.xdsp_address = (uint64_t)xdsp_address;
  // text_output_printf("Locating ACPI Tables...");

  text_output_printf("XDSP Address: 0x%x\n", xdsp_address);

  XDSP *xdsp = (XDSP *)xdsp_address;
  if (!validate_xdsp(xdsp)) {
    panic("\nXDSP validation failed!\n");
  }

  acpi.xsdt = (XSDT *)xdsp->xsdt_phys;

  if (!validate_acpi_checksum(acpi.xsdt, acpi.xsdt->header.Length)) {
    panic("\nXDST validation failed!\n");
  }

  // text_output_printf("Done\n");

  // int entries = (acpi.xsdt->header.Length - sizeof(acpi.xsdt->header)) / sizeof(ACPISDTHeader *);

  // text_output_printf("ACPI Tables: ");
  // for (int i = 0; i < entries; i++) {
  //   // TODO: Figure out why this works
  //   ACPISDTHeader *header = (&acpi.xsdt->table_pointers)[i];
  //   text_output_printf("%c%c%c%c ", header->Signature[0], header->Signature[1], header->Signature[2], header->Signature[3]);
  // }
  // text_output_printf("\n");


  // FADT *fadt = (FADT *)acpi_locate_table("FACP");
  // text_output_printf("FADT Table: 0x%x\n", fadt);

  // // TODO: Determine why we cannot use the X_Dsdt field (64-bit pointer)
  // ACPISDTHeader *dsdt_header = (ACPISDTHeader *)(uint64_t)fadt->Dsdt;
  // text_output_printf("DSDT Table: 0x%x\n", dsdt_header);

  // text_output_printf("DSDT length: %d, verified? %d\n", dsdt_header->Length, validate_acpi_checksum(dsdt_header, dsdt_header->Length));

  

}
