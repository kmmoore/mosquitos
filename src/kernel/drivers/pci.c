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

static void print_pci_device(PCIDevice *device) {
  text_output_printf("[PCI Device 0x%02x 0x%02x 0x%02x] Class Code: 0x%02x, Subclass: 0x%02x, Program IF: 0x%02x, Multifunction? %d\n", device->bus, device->device, device->function, device->class_code, device->subclass, device->program_if, device->multifunction);

}

static PCIDevice * add_pci_device(uint8_t bus, uint8_t device, uint8_t function) {
  uint32_t vendor_word = PCI_HEADER_READ_FIELD_WORD(bus, device, function, vendor_id);
  if (PCI_HEADER_FIELD_IN_WORD(vendor_word, vendor_id) != 0xffff) {
    PCIDevice *new_device = &pci_data.devices[pci_data.num_devices++];
    new_device->bus = bus;
    new_device->device = device;
    new_device->function = function;

    uint32_t class_field = PCI_HEADER_READ_FIELD_WORD(bus, device, function, class_code);

    new_device->class_code = PCI_HEADER_FIELD_IN_WORD(class_field, class_code);
    new_device->subclass = PCI_HEADER_FIELD_IN_WORD(class_field, subclass);
    new_device->program_if = PCI_HEADER_FIELD_IN_WORD(class_field, program_if);

    uint32_t htype_field = PCI_HEADER_READ_FIELD_WORD(bus, device, function, header_type);

    new_device->header_type = PCI_HEADER_FIELD_IN_WORD(htype_field, header_type);
    new_device->multifunction = (new_device->header_type & (1 << 7)) > 0;
    new_device->header_type = new_device->header_type & ~(1 << 7);

    print_pci_device(new_device);

    return new_device;
  }

  return NULL;
}


static void enumerate_devices() {
  for(int bus = 0; bus < 256; bus++) {
    for(int device = 0; device < 32; device++) {
      PCIDevice *new_device = add_pci_device(bus, device, 0);

      if (new_device && new_device->multifunction) {
        for (int func = 1; func < 8; ++func) {
          add_pci_device(new_device->bus, new_device->device, func);
        }
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

