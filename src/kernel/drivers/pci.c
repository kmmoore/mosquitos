#include <kernel/drivers/pci.h>
#include <kernel/drivers/text_output.h>
#include <acpi.h>

#define PCI_MAX_BUS_NUM 256
#define PCI_MAX_SLOT_NUM 32
#define PCI_MAX_FUNCTION_NUM 8
#define PCI_NUM_INTERRUPT_PORTS 4

#define PCI_MAX_DEVICES 20
#define PCI_MAX_DRIVERS (2*PCI_MAX_DEVICES)

typedef union {

  struct {
    uint8_t offset;
    uint8_t function_number:3;
    uint8_t slot:5;
    uint8_t bus_number;
    uint8_t reserved:7;
    uint8_t enable:1;
  } __attribute__((packed)) svalue;
  uint32_t ivalue;

} PCIConfigAddress;

static struct {
  PCIDevice devices[PCI_MAX_DEVICES];
  uint32_t irq_routing_table[PCI_MAX_SLOT_NUM][4]; // TODO: Support more than just bus 0
  int num_devices;

  PCIDeviceDriverInterface drivers[PCI_MAX_DRIVERS];
  int num_drivers;
} pci_data;

uint32_t pci_config_read_word (uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
  PCIConfigAddress address;
  address.svalue.offset = offset;
  address.svalue.function_number = func;
  address.svalue.slot = slot;
  address.svalue.bus_number = bus;
  address.svalue.reserved = 0;
  address.svalue.enable = 1;

  io_write_32(0xCF8, address.ivalue);
  return io_read_32(0xCFC);
}

void pci_config_write_word (uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t value) {
  PCIConfigAddress address;
  address.svalue.offset = offset;
  address.svalue.function_number = func;
  address.svalue.slot = slot;
  address.svalue.bus_number = bus;
  address.svalue.reserved = 0;
  address.svalue.enable = 1;

  io_write_32(0xCF8, address.ivalue);
  return io_write_32(0xCFC, value);
}


ACPI_STATUS acpi_system_bus_walk_callback(ACPI_HANDLE Object, UINT32 NestingLevel UNUSED, void *Context UNUSED, void **ReturnValue UNUSED) {
  ACPI_DEVICE_INFO *device_info;
  assert(AcpiGetObjectInfo(Object, &device_info) == AE_OK);

  if (device_info->Flags == ACPI_PCI_ROOT_BRIDGE) { 
    // TODO figure out how to deal with multiple buses

    ACPI_PCI_ROUTING_TABLE routing_table[PCI_MAX_SLOT_NUM * PCI_NUM_INTERRUPT_PORTS];
      ACPI_BUFFER buffer;
      buffer.Length = sizeof(routing_table);
      buffer.Pointer = &routing_table;
      assert(ACPI_SUCCESS(AcpiGetIrqRoutingTable(Object, &buffer)));
      assert(routing_table[0].Length == sizeof(ACPI_PCI_ROUTING_TABLE));

      for (int i = 0; routing_table[i].Length > 0; ++i) {

        uint16_t slot_number = routing_table[i].Address >> 16;

        // TODO: Make sure the SourceIndex isn't referencing a link device
        pci_data.irq_routing_table[slot_number][routing_table[i].Pin] = routing_table[i].SourceIndex;
      }
  }

  ACPI_FREE(device_info);

  return AE_OK;
}

static void pci_load_irq_routing_table() {
  ACPI_HANDLE system_bus_handle;
  ACPI_STATUS status = AcpiGetHandle(NULL, "\\_SB", &system_bus_handle);
  assert(status == AE_OK);

  void *walk_return_value;
  status = AcpiWalkNamespace(ACPI_TYPE_DEVICE, system_bus_handle, 1, acpi_system_bus_walk_callback, NULL, NULL, &walk_return_value);
  assert(status == AE_OK);
}

UNUSED static void print_pci_device(PCIDevice *device) {
  text_output_printf("[PCI Device 0x%02x 0x%02x 0x%02x] Class Code: 0x%02x, Subclass: 0x%02x, Program IF: 0x%02x, IRQ #: %d, Multifunction? %d\n", device->bus, device->slot, device->function, device->class_code, device->subclass, device->program_if, device->real_irq, device->multifunction);

}

static PCIDeviceDriverInterface *driver_for_device(PCIDevice *device) {
  for (int i = 0; i < pci_data.num_drivers; ++i) {
    PCIDeviceDriverInterface *driver = &pci_data.drivers[i];
    if (driver->class_code == device->class_code &&
        driver->subclass == device->subclass &&
        driver->program_if == device->program_if) {
      return driver;
    }
  }

  return NULL;
}

static PCIDevice * add_pci_device(uint8_t bus, uint8_t slot, uint8_t function) {
  uint32_t vendor_word = PCI_HEADER_READ_FIELD_WORD(bus, slot, function, vendor_id);
  if (PCI_HEADER_FIELD_IN_WORD(vendor_word, vendor_id) != 0xffff) {
    PCIDevice *new_device = &pci_data.devices[pci_data.num_devices++];
    new_device->bus = bus;
    new_device->slot = slot;
    new_device->function = function;

    uint32_t class_field = PCI_HEADER_READ_FIELD_WORD(bus, slot, function, class_code);

    new_device->class_code = PCI_HEADER_FIELD_IN_WORD(class_field, class_code);
    new_device->subclass = PCI_HEADER_FIELD_IN_WORD(class_field, subclass);
    new_device->program_if = PCI_HEADER_FIELD_IN_WORD(class_field, program_if);

    uint32_t htype_field = PCI_HEADER_READ_FIELD_WORD(bus, slot, function, header_type);

    new_device->header_type = PCI_HEADER_FIELD_IN_WORD(htype_field, header_type);
    new_device->multifunction = (new_device->header_type & (1 << 7)) > 0;
    new_device->header_type = new_device->header_type & ~(1 << 7);

    uint32_t interrupt_field = PCI_HEADER_READ_FIELD_WORD(bus, slot, function, interrupt_pin);
    uint8_t interrupt_pin = PCI_HEADER_FIELD_IN_WORD(interrupt_field, interrupt_pin);

    if (slot < 2 || bus > 0) text_output_printf("Loading incorrect IRQ #\n");
    if (interrupt_pin == 0) {
      new_device->has_interrupts = 0;
    } else {
      new_device->has_interrupts = 1;
      new_device->real_irq = pci_data.irq_routing_table[slot][interrupt_pin-1]; // INTA# is 0x01
    }

    PCIDeviceDriverInterface *driver = driver_for_device(new_device);
    if (driver != NULL) {
      new_device->driver = *driver;
      new_device->has_driver = true;
      new_device->driver.device = new_device;
      new_device->driver.init(&new_device->driver);
    } else {
      new_device->has_driver = false;
    }

    return new_device;
  }

  return NULL;
}


void pci_enumerate_devices() {
  REQUIRE_MODULE("pci");
  pci_data.num_devices = 0;

  for(int bus = 0; bus < PCI_MAX_BUS_NUM; bus++) {
    for(int slot = 0; slot < PCI_MAX_SLOT_NUM; slot++) {
      PCIDevice *new_device = add_pci_device(bus, slot, 0);

      if (new_device && new_device->multifunction) {
        for (int func = 1; func < PCI_MAX_FUNCTION_NUM; ++func) {
          add_pci_device(bus, slot, func);
        }
      }
    }
  }
}

void pci_init() {
  REQUIRE_MODULE("interrupt");
  REQUIRE_MODULE("acpi_full");

  pci_data.num_drivers = 0;
  
  pci_load_irq_routing_table();

  REGISTER_MODULE("pci");
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

void pci_register_device_driver(PCIDeviceDriverInterface driver_interface) {
  REQUIRE_MODULE("pci");

  assert(pci_data.num_drivers < PCI_MAX_DRIVERS);
  pci_data.drivers[pci_data.num_drivers++] = driver_interface;

  text_output_printf("Registered PCI device driver for cc: %d, sc: %d, pif: %d\n", driver_interface.class_code, driver_interface.subclass, driver_interface.program_if);
}
