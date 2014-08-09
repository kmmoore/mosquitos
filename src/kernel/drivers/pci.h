#include "../kernel_common.h"
#include "../util.h"

#ifndef _PCI_H
#define _PCI_H

typedef struct {
  uint16_t vendor_id, device_id;
  uint16_t command, status;
  uint8_t revision_id, program_if, subclass, class_code;
  uint8_t cache_line_size, latency_timer, header_type, bist;

  uint32_t base_address_0;
  uint32_t base_address_1;
  uint32_t base_address_2;
  uint32_t base_address_3;
  uint32_t base_address_4;
  uint32_t base_address_5;

  uint32_t cardbus_cis_pointer;
  uint16_t subsystem_vendor_id, subsystem_id;
  uint32_t expansion_rom_base_address;
  uint8_t capability_pointer;
  uint32_t reserved:24;
  uint32_t reserved2;

  uint8_t interrupt_line, interrupt_pin, min_grant, max_latency;
} __attribute__((packed)) PCIGenericConfigHeader;

typedef struct {
  uint8_t program_if, subclass, class_code;
  uint8_t bus, device;
  uint8_t multifunction, header_type;
} PCIDevice;

// Functions  

void pci_init();
uint32_t pci_config_read_word (uint8_t bus, uint8_t device_number, uint8_t func, uint8_t offset);
PCIDevice * pci_find_device(uint8_t class_code, uint8_t subclass, uint8_t program_if);

// Macros
#define PCI_OFFSET_FOR_HDR_FIELD(field) (offsetof(PCIGenericConfigHeader, field) & ~0b11)
#define PCI_OFFSET_WITHIN_HDR_FIELD(field) (offsetof(PCIGenericConfigHeader, field) - PCI_OFFSET_FOR_HDR_FIELD(field))

#define PCI_HEADER_FIELD_IN_WORD(word, field) (field_in_word(word, PCI_OFFSET_WITHIN_HDR_FIELD(field), member_size(PCIGenericConfigHeader, field)))
#define PCI_HEADER_READ_FIELD_WORD(bus, device, func, field) (pci_config_read_word(bus, device, func, PCI_OFFSET_FOR_HDR_FIELD(field)))

#endif