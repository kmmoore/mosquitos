#include "pci.h"
#include "text_output.h"

#define PCI_MAX_DEVICES 20

typedef union {

  struct {
    uint8_t offset;
    uint8_t function_number:3;
    uint8_t device:5;
    uint8_t bus_number;
    uint8_t reserved:7;
    uint8_t enable:1;
  } __attribute__((packed)) svalue;
  uint32_t ivalue;

} PCIConfigAddress;

static struct {
  PCIDevice devices[PCI_MAX_DEVICES];
  int num_devices;
} pci_data;

uint32_t pci_config_read_word (uint8_t bus, uint8_t device, uint8_t func, uint8_t offset) {
  PCIConfigAddress address;
  address.svalue.offset = offset;
  address.svalue.function_number = func;
  address.svalue.device = device;
  address.svalue.bus_number = bus;
  address.svalue.reserved = 0;
  address.svalue.enable = 1;

  io_write_32(0xCF8, address.ivalue);
  return io_read_32(0xCFC);
}

void pci_config_write_word (uint8_t bus, uint8_t device, uint8_t func, uint8_t offset, uint32_t value) {
  PCIConfigAddress address;
  address.svalue.offset = offset;
  address.svalue.function_number = func;
  address.svalue.device = device;
  address.svalue.bus_number = bus;
  address.svalue.reserved = 0;
  address.svalue.enable = 1;

  io_write_32(0xCF8, address.ivalue);
  return io_write_32(0xCFC, value);
}

static void enumerate_devices() {
  for(int bus = 0; bus < 256; bus++) {
    for(int device = 0; device < 32; device++) {
      uint32_t vendor_word = PCI_HEADER_READ_FIELD_WORD(bus, device, 0, vendor_id);
      if (PCI_HEADER_FIELD_IN_WORD(vendor_word, vendor_id) != 0xffff) {
        PCIDevice *new_device = &pci_data.devices[pci_data.num_devices++];
        new_device->bus = bus;
        new_device->device = device;

        uint32_t class_field = PCI_HEADER_READ_FIELD_WORD(bus, device, 0, class_code);

        new_device->class_code = PCI_HEADER_FIELD_IN_WORD(class_field, class_code);
        new_device->subclass = PCI_HEADER_FIELD_IN_WORD(class_field, subclass);
        new_device->program_if = PCI_HEADER_FIELD_IN_WORD(class_field, program_if);

        uint32_t htype_field = PCI_HEADER_READ_FIELD_WORD(bus, device, 0, header_type);

        new_device->header_type = PCI_HEADER_FIELD_IN_WORD(htype_field, header_type);
        new_device->multifunction = (new_device->header_type & (1 << 7)) > 0;
        new_device->header_type = new_device->header_type & ~(1 << 7);

        // text_output_printf("PCI: bus: 0x%x 0x%x, 0x%x, 0x%x, mf: %d\n", bus, new_device->class_code, new_device->subclass, new_device->program_if, new_device->multifunction);
        // text_output_printf("Vendor ID: 0x%x, Device ID: 0x%x\n", PCI_HEADER_FIELD_IN_WORD(vendor_word, vendor_id), PCI_HEADER_FIELD_IN_WORD(vendor_word, device_id));
      }
    }
  }
}

void pci_init() {
  // text_output_printf("Enumerating PCI devices...");
  enumerate_devices();
  // text_output_printf("Done\n");
}

PCIDevice * pci_find_device(uint8_t class_code, uint8_t subclass, uint8_t program_if) {
  for (int i = 0; i < pci_data.num_devices; ++i) {
    PCIDevice *device = &pci_data.devices[i];
    if (device->class_code == class_code && device->subclass == subclass && device->program_if == program_if) {
      return device;
    }
  }

  return NULL;
}

