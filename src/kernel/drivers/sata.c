#include "sata.h"
#include "text_output.h"
#include "pci.h"
#include "../util.h"

#define PCI_MASS_STORAGE_CLASS_CODE 0x01
#define PCI_SATA_SUBCLASS 0x06
#define PCI_AHCI_V1_PROGRAM_IF 0x01

typedef struct {
  int r;
} __attribute__((packed)) SATACommandField;

struct {
  PCIDevice *hba;
} sata_data;

void sata_init() {
  // Find a SATA AHCI v1 PCI device
  sata_data.hba = pci_find_device(PCI_MASS_STORAGE_CLASS_CODE, PCI_SATA_SUBCLASS, PCI_AHCI_V1_PROGRAM_IF);

  if (!sata_data.hba) {
    panic("Could not find SATA AHCI PCI device!\n");
  }

  // We only know how to deal with single-function general devices
  assert(sata_data.hba->multifunction == false);
  assert(sata_data.hba->header_type == 0x0);

  text_output_printf("Found SATA AHCI controller\n");

  uint32_t capability_word = PCI_HEADER_READ_FIELD_WORD(sata_data.hba->bus, sata_data.hba->device, 0, capability_pointer);
  uint8_t capability_pointer = PCI_HEADER_FIELD_IN_WORD(capability_word, capability_pointer);

  text_output_printf("SATA capability_pointer: 0x%x\n", capability_pointer);

  while (true) {
    uint32_t capability = pci_config_read_word(sata_data.hba->bus, sata_data.hba->device, 0, capability_pointer);

    text_output_printf("Capability: 0x%x\n", field_in_word(capability_pointer, 0, 1));

    capability_pointer = field_in_word(capability, 3, 1);
    text_output_printf("Next: 0x%x\n", capability_pointer);
    if (capability_pointer == 0) break;
  }

}
