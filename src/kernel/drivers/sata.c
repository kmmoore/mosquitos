#include "sata.h"
#include "text_output.h"
#include "pci.h"
#include "../util.h"

#define PCI_MASS_STORAGE_CLASS_CODE 0x01
#define PCI_SATA_SUBCLASS 0x06
#define PCI_AHCI_V1_PROGRAM_IF 0x01

void sata_init() {
  // Find a SATA AHCI v1 PCI device
  PCIDevice *sata_hba = pci_find_device(PCI_MASS_STORAGE_CLASS_CODE, PCI_SATA_SUBCLASS, PCI_AHCI_V1_PROGRAM_IF);

  if (!sata_hba) {
    panic("Could not find SATA AHCI PCI device!\n");
  }

  text_output_printf("Found SATA AHCI controller, multifunction: %s\n", sata_hba->multifunction ? "yes" : "no");
}
