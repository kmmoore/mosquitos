#include "acpi.h"
#include "text_output.h"
#include "util.h"
#include "../common/mem_util.h"

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

typedef struct {
  ACPISDTHeader header;
  ACPISDTHeader *table_pointers;
} __attribute__((packed)) XSDT;

static struct {
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

void acpi_init(void *xdsp_address) {
  text_output_printf("Locating ACPI Tables...");

  XDSP *xdsp = (XDSP *)xdsp_address;
  if (!validate_xdsp(xdsp)) {
    panic("\nXDSP validation failed!\n");
  }

  acpi.xsdt = (XSDT *)xdsp->xsdt_phys;

  if (!validate_acpi_checksum(acpi.xsdt, acpi.xsdt->header.Length)) {
    panic("\nXDST validation failed!\n");
  }

  text_output_printf("Done\n");

  int entries = (acpi.xsdt->header.Length - sizeof(acpi.xsdt->header)) / sizeof(ACPISDTHeader *);

  text_output_printf("ACPI Tables: ");
  for (int i = 0; i < entries; i++) {
    // TODO: Figure out why this works
    ACPISDTHeader *header = (&acpi.xsdt->table_pointers)[i];
    text_output_printf("%c%c%c%c ", header->Signature[0], header->Signature[1], header->Signature[2], header->Signature[3]);
  }
  text_output_printf("\n");

}
