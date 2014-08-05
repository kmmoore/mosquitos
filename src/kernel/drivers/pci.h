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
  uint8_t primary_bus_number, secondary_bus_number, subcoordinate_bus_number, secondary_latency_timer;
  uint16_t io_base:8, io_limit:8, secondary_status;
  uint16_t memory_base, memory_limit;
  uint16_t prefetchable_memory_base, prefetchable_memory_limit;
  uint32_t prefetchable_base_upper;
  uint32_t prefetchable_limit_upper;
  uint16_t io_base_upper, io_limit_upper;
  uint32_t capability_pointer:8, reserved:24;
  uint32_t expansion_rom_base_address;
  uint16_t interrupt_line:8, interrupt_pin:8, bridge_control;
} __attribute__((packed)) PCIConfigurationHeader;

typedef struct {
  uint8_t program_if, subclass, class_code;
  uint8_t bus, device_number;
  uint8_t multifunction, header_type;
} PCIDevice;

// Functions  

void pci_init();
uint32_t pci_config_read_word (uint8_t bus, uint8_t device_number, uint8_t func, uint8_t offset);
PCIDevice * pci_find_device(uint8_t class_code, uint8_t subclass, uint8_t program_if);

// Macros
#define PCI_HEADER_FIELD_IN_WORD(word, field) (field_in_word(word, OFFSET_WITHIN_HDR_FIELD(field), member_size(PCIConfigurationHeader, field)))
#define PCI_HEADER_READ_FIELD_WORD(bus, device, func, field) (pci_config_read_word(bus, device, func, OFFSET_FOR_HDR_FIELD(field)))

#endif