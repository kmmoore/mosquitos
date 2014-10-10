#include "sata.h"
#include "text_output.h"
#include "pci.h"
#include "../util.h"
#include "timer.h"

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

  uint32_t abar_word = pci_config_read_word(sata_data.hba->bus, sata_data.hba->slot, sata_data.hba->function, 0x24);
  uintptr_t hba_base_address = (abar_word & FIELD_MASK(19, 13));

  text_output_printf("HBA Base Address: %p\n", hba_base_address);

  uint32_t ahci_version_word = *(uint32_t *)(hba_base_address + 0x10);
  text_output_printf("AHCI Version: 0x%x\n", ahci_version_word);

  text_output_printf("HBA Capabilities: 0b%b\n", *(uint32_t *)(hba_base_address + 0x00));

  uint32_t hba_controls = *(uint32_t *)(hba_base_address + 0x04);
  text_output_printf("Global HBA Controls: 0b%b\n", hba_controls);

  *(uint32_t *)(hba_base_address + 0x04) = hba_controls & (1 << 0); // Reset HBA
  timer_thread_sleep(10);
  *(uint32_t *)(hba_base_address + 0x04) = hba_controls & (1 << 31); // Enable ACHI mode
  *(uint32_t *)(hba_base_address + 0x04) = hba_controls & (1 << 1); // Enable interrupts
  timer_thread_sleep(10); 

  hba_controls = *(uint32_t *)(hba_base_address + 0x04);
  text_output_printf("Global HBA Controls: 0b%b\n", hba_controls);

  uint32_t ports_implemented = *(uint32_t *)(hba_base_address + 0x0C);

  text_output_printf("Ports implemented: 0b%b\n", ports_implemented);
}
