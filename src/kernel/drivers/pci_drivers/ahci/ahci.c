#include <kernel/drivers/pci_drivers/ahci/ahci.h>
#include <kernel/drivers/pci_drivers/ahci/ahci_internal.h>
#include <kernel/drivers/pci_drivers/ahci/sata.h>
#include <kernel/drivers/text_output.h>
#include <kernel/drivers/pci.h>
#include <kernel/drivers/interrupt.h>
#include <kernel/drivers/apic.h>
#include <kernel/util.h>
#include <kernel/drivers/timer.h>
#include <common/mem_util.h>
#include <kernel/memory/kmalloc.h>

#include <kernel/threading/mutex/semaphore.h>

// TOOD: Make sure we don't use PCI/SATA MMIO/DMA space for other stuff

#define SATA_IV 39

#define MAX_CACHED_DEVICES 8

#define PCI_MASS_STORAGE_CLASS_CODE 0x01
#define PCI_SATA_SUBCLASS           0x06
#define PCI_AHCI_V1_PROGRAM_IF      0x01

#define AHCI_SIG_SATA    0x00000101  // SATA drive
#define AHCI_SIG_ATAPI  0xEB140101  // SATAPI drive
#define AHCI_SIG_SEMB   0xC33C0101  // Enclosure management bridge
#define AHCI_SIG_PM     0x96690101  // Port multiplier

#define AHCI_DEV_BUSY (1 << 7)
#define AHCI_DEV_DRQ  (1 << 3)

#define MAX_BYTES_PER_PRDT (1 << 22)

#define COMMAND_TIMEOUT_MS 500

static AHCIDeviceType device_type_in_port(HBAPort *port);

typedef struct _AHCIDevice {
  uint8_t port_number;
  AHCIDeviceType type;
  Semaphore pending_command;
  PCIDeviceDriver *driver;
} AHCIDevice;

typedef struct _AHCIData {
  // TODO: Store HBA(s?) capabilities
  HBAMemory *hba;
  bool use_64_bits;
  AHCIDevice devices[MAX_CACHED_DEVICES];
  uint8_t num_devices;
} AHCIData;

static inline HBAPort * port_from_device(AHCIDevice *device) {
  assert(device);
  assert(device->driver);
  assert(device->driver->driver_data);

  AHCIData *ahci_data = (AHCIData *)device->driver->driver_data;
  assert(ahci_data->hba->ports);
  return &ahci_data->hba->ports[device->port_number];
}

inline AHCIDeviceType ahci_device_type(AHCIDevice *device) {
  return device->type;
}

static void print_ahci_device(AHCIDevice *device) {
  char *type = "Unknown";
  switch (ahci_device_type(device)) {
    case AHCI_DEVICE_NONE: type = "None"; break;
    case AHCI_DEVICE_SATA: type = "SATA"; break;
    case AHCI_DEVICE_SATAPI: type = "SATAPI"; break;
    case AHCI_DEVICE_SEMB: type = "SEMB"; break;
    case AHCI_DEVICE_PM: type = "Port Multiplier"; break;
    case AHCI_DEVICE_UNKNOWN: type = "Unknown"; break;
  }

  text_output_printf("[AHCI Device] Port #: %d, Type: %s\n", device->port_number, type);
  if (ahci_device_type(device) == AHCI_DEVICE_SATA) {
    text_output_printf("Signature: 0b%b\n", field_in_word(port_from_device(device)->signature, 0, 1));
  }
}

void ahci_clear_pending_interrupts(AHCIDevice *device) {
  HBAPort *port = port_from_device(device);
  port->interrupt_status = 0;
}

void port_start(AHCIDevice *device) {
  HBAPort *port = port_from_device(device);
  // Set up the HBA to process a command (AHCI Spec. p.26)
  port->command |= (1 << 4); // Enable FIS receiving
  port->command |= (1 << 0); // Start port
  port->interrupt_enable = ALL_ONES; // Enable all interrupts

  assert((port->command & (1 << 4)) != 0);
  assert((port->command & (1 << 0)) != 0);
}

void port_stop(AHCIDevice *device) {
  HBAPort *port = port_from_device(device);
  // Disable command processing
  port->command &= ~(1 << 0); // Stop port
  assert((port->command & (1 << 0)) == 0);
}

// Find a free command list slot
int ahci_find_command_slot(AHCIDevice *device) {
  HBAPort *port = port_from_device(device);

  // If bit isn't set in SACT and CI, the slot is free
  uint32_t slots = (port->sata_active | port->command_issue);
  for (int i = 0; i < 32; i++) {
    if ((slots & (1 << i)) == 0) {
      return i;
    }
  }

  text_output_printf("Cannot find free command list entry\n");
  return -1;
}

FISRegisterH2D * ahci_initialize_command_fis(PCIDeviceDriver *driver, AHCIDevice *device, int slot, bool write, bool prefetchable, uint64_t byte_size, uint8_t *dma_buffer) {
  HBAPort *port = port_from_device(device);

  bool use_64_bits = ((AHCIData *)driver->driver_data)->use_64_bits;

  // Get the command header associated with our free slot
  // TODO: Refactor this into a #define or something
  uintptr_t command_header_address = (uintptr_t)port->command_list_base_address;
  if (use_64_bits) {
    command_header_address |= ((uintptr_t)port->command_list_base_address_upper) << 32;
  }
  command_header_address += slot * sizeof(HBACommandHeader);

  HBACommandHeader *command_header = (HBACommandHeader *)command_header_address;
  memset(command_header, 0, sizeof(HBACommandHeader));
  command_header->command_fis_length = sizeof(FISRegisterH2D) / sizeof(uint32_t); // Command FIS size in dwords
  command_header->write = write;
  command_header->prefetchable = prefetchable;
  command_header->prdt_count = ((byte_size - 1) / MAX_BYTES_PER_PRDT) + 1;

  // Get command table from command header
  uintptr_t command_table_address = (uintptr_t)command_header->command_table_base_address;
  if (use_64_bits) {
    command_table_address |= ((uintptr_t)command_header->command_table_base_address_upper) << 32;
  }

  HBACommandTable *command_table = (HBACommandTable *)command_table_address;
  memset(command_table, 0, sizeof(HBACommandTable) + (command_header->prdt_count * sizeof(HBAPRDTEntry)));

  // Get PRDT from command table
  HBAPRDTEntry *prdt = command_table->prdt_entry;
  
  // Fill PRDT. 4MB (8192 sectors) per PRDT entrt
  for (int i = 0; i < command_header->prdt_count; ++i) {
    prdt[i].data_base_address = field_in_word((uint64_t)dma_buffer, 0, 4);
    prdt[i].data_base_address_upper = field_in_word((uint64_t)dma_buffer, 4, 4);
    // data_size is 0-indexed
    prdt[i].data_size = byte_size > MAX_BYTES_PER_PRDT ? MAX_BYTES_PER_PRDT - 1 : byte_size - 1;
    prdt[i].interrupt_on_completion = 1;
    dma_buffer += prdt[i].data_size + 1;
    byte_size -= prdt[i].data_size + 1;
  }
  
  // Setup command
  FISRegisterH2D *command_fis = (FISRegisterH2D*)(uintptr_t)(&command_table->cfis);
  memset(command_fis, 0, sizeof(FISRegisterH2D));
  
  command_fis->fis_type = FIS_TYPE_REG_H2D;
  command_fis->c = 1; // Command FIS

  return command_fis;
}

// Attempt to issue command, true when completed, false if error
bool ahci_issue_command(AHCIDevice *device, int slot) {
  HBAPort *port = port_from_device(device);

  // Wait until the port is free
  int spin = 0;
  while ((port->tfd & (AHCI_DEV_BUSY | AHCI_DEV_DRQ)) && spin < 1000000) {
    spin++;
  }

  if (spin == 1000000) {
    text_output_printf("Port is hung\n");
    return false;
  }

  // Issue command
  port->command_issue |= 1 << slot;

  // Make sure the command has actually finished
  while (port->command_issue & (1 << slot)) {
    // Sleep until we get an interrupt or we timeout
    if (!semaphore_down(&device->pending_command, 1, COMMAND_TIMEOUT_MS)) {
      text_output_printf("AHCI command issue timeout.\n");
      return false;
    }
  }

  return true;
}

static bool reset_hba(HBAMemory *hba) {
  hba->ghc |= (1 << 0); // Reset HBA
  while (hba->ghc & 0x01) __asm__ volatile ("nop"); // Wait for reset

  hba->ghc |= (1 << 31); // Enable ACHI mode
  hba->ghc |= (1 << 1); // Set interrupt enable

  return (hba->ghc & 0x01) == 0;
}

static void enumerate_devices(PCIDeviceDriver *driver, HBAMemory *hba, AHCIData *ahci_data) {
  // Check all implemented ports
  uint32_t ports_implemented = hba->ports_implemented;
  for (int i = 0; i < 32; ++i) {
    if ((ports_implemented & (1 << i)) > 0) {
      AHCIDeviceType device_type = device_type_in_port(&hba->ports[i]);

      // Keep track of all ports with devices in them
      if (device_type != AHCI_DEVICE_NONE) {
        assert(ahci_data->num_devices < MAX_CACHED_DEVICES);
        AHCIDevice *new_device = &ahci_data->devices[ahci_data->num_devices++];

        new_device->driver = driver;
        new_device->port_number = i;
        new_device->type = device_type;

        semaphore_init(&new_device->pending_command, 0);
      }
    }
  }
}
 
static AHCIDeviceType device_type_in_port(HBAPort *port) {
  // Determine type of device in the port
  uint32_t sata_status = port->sata_status;
 
  uint8_t interface_state = (sata_status >> 8) & 0x0F;
  uint8_t device_state = sata_status & 0x0F;
 
  if (device_state != DET_PRESENT || interface_state != IPM_ACTIVE) return AHCI_DEVICE_NONE;
 
  switch (port->signature) {
    case AHCI_SIG_ATAPI: return AHCI_DEVICE_SATAPI;
    case AHCI_SIG_SEMB:  return AHCI_DEVICE_SEMB;
    case AHCI_SIG_PM:    return AHCI_DEVICE_PM;
    case AHCI_SIG_SATA:  return AHCI_DEVICE_SATA;
    default:             return AHCI_DEVICE_UNKNOWN;
  }
}

static bool initialize_hba(PCIDeviceDriver *driver) {
  PCIDevice *hba_device = driver->device;

  assert(hba_device->class_code == PCI_MASS_STORAGE_CLASS_CODE);
  assert(hba_device->subclass == PCI_SATA_SUBCLASS);
  assert(hba_device->program_if == PCI_AHCI_V1_PROGRAM_IF);

  // We only know how to deal with general devices
  assert(hba_device->header_type == 0x0);

  // TODO: Fall back to polling if there isn't a PCI interrupt associated. In reality this should never(?) happen.
  assert(hba_device->has_interrupts);

  uint32_t cmd_word = PCI_HEADER_READ_FIELD_WORD(hba_device->bus, hba_device->slot, hba_device->function, command);
  text_output_printf("HBA PCI CMD Field: 0b%b\n", PCI_HEADER_FIELD_IN_WORD(cmd_word, command) & (1 << 10));

  text_output_printf("HBA Slot: %d, IRQ #: %d\n", hba_device->slot, hba_device->real_irq);

  uint32_t abar_word = pci_config_read_word(hba_device->bus, hba_device->slot, hba_device->function, 0x24);
  uintptr_t hba_base_address = (abar_word & FIELD_MASK(19, 13));
  HBAMemory *hba = (HBAMemory *)hba_base_address;

  uint16_t major_version = field_in_word(hba->version, 2, 2);
  uint16_t minor_version = field_in_word(hba->version, 1, 1);
  uint16_t subminor_version = field_in_word(hba->version, 0, 1);
  text_output_printf("HBA AHCI Version: %d.%d.%d\n", major_version, minor_version, subminor_version);

  assert(major_version == 1 && minor_version <= 3); // We only know how to deal with ACHI version 1.0 - 1.3

  text_output_printf("HBA Capabilities: 64-bit? %d, SSS? %d, AHCI Only? %d, num cmd slots: %d\n", (hba->capabilities & (1 << 31)) > 0, (hba->capabilities & (1 << 27)) > 0, (hba->capabilities & (1 << 18)) > 0, ((hba->capabilities & FIELD_MASK(5, 8)) >> 8) + 1);

  assert(reset_hba(hba));


  driver->driver_data = kcalloc(1, sizeof(AHCIData));
  AHCIData *ahci_data = (AHCIData *)(driver->driver_data);
  ahci_data->use_64_bits = (hba->capabilities & (1 << 31)) > 0;
  ahci_data->hba = hba;


  enumerate_devices(driver, hba, ahci_data);

  for (int i = 0; i < ahci_data->num_devices; ++i) {
    print_ahci_device(&ahci_data->devices[i]);
  }

  return true;
}

static void ahci_driver_init(PCIDeviceDriver *driver) {
  text_output_printf("Initializing AHCI driver for PCI device: %p\n");

  if (!initialize_hba(driver)) {
    panic("Could not initialize HBA!\n");
  }
}

static PCIDeviceDriverError ahci_execute_command(PCIDeviceDriver *driver,
                                                   uint64_t command_id, void *input_buffer,
                                                   uint64_t input_buffer_size, void *output_buffer,
                                                   uint64_t output_buffer_size) {

  AHCIData *ahci_data = (AHCIData *)driver->driver_data;
  
  switch ((enum AHCICommandIDs)command_id) {
    case AHCI_COMMAND_LIST_DEVICES:
      text_output_printf("List devices\n");
      break;

    case AHCI_COMMAND_IDENTIFY:
    {
      assert(input_buffer_size == sizeof(struct AHCIIdentifyCommand));
      struct AHCIIdentifyCommand *command = (struct AHCIIdentifyCommand *)input_buffer;
      if (command->device_id >= ahci_data->num_devices) {
        return PCI_ERROR_INVALID_PARAMETERS;
      }

      AHCIDevice *device = &ahci_data->devices[command->device_id];

      switch (ahci_device_type(device)) {
        case AHCI_DEVICE_SATA:
          return sata_identify(driver, device, output_buffer, output_buffer_size);

        default:
          NOT_IMPLEMENTED
      }
    }
    break;

    case AHCI_COMMAND_READ:
    {
      assert(input_buffer_size == sizeof(struct AHCIReadCommand));
      struct AHCIReadCommand *command = (struct AHCIReadCommand *)input_buffer;
      if (command->device_id >= ahci_data->num_devices) {
        return PCI_ERROR_INVALID_PARAMETERS;
      }

      AHCIDevice *device = &ahci_data->devices[command->device_id];

      switch (ahci_device_type(device)) {
        case AHCI_DEVICE_SATA:
          return sata_read(driver, device, command->address, command->block_count,
                           output_buffer, output_buffer_size);

        default:
          NOT_IMPLEMENTED
      }
    }
    break;

    case AHCI_COMMAND_WRITE:
    {
      assert(input_buffer_size == sizeof(struct AHCIWriteCommand));
      struct AHCIWriteCommand *command = (struct AHCIWriteCommand *)input_buffer;
      if (command->device_id >= ahci_data->num_devices) {
        return PCI_ERROR_INVALID_PARAMETERS;
      }

      // AHCIDevice *device = &ahci_data->devices[command->device_id];

      NOT_IMPLEMENTED
    }
    break;
  }

  return PCI_ERROR_COMMAND_NOT_IMPLEMENTED;
}

static void ahci_isr(PCIDeviceDriver *driver) {
  AHCIData *ahci_data = (AHCIData *)driver->driver_data;
  if (ahci_data->hba->interrupt_status == 0) return;
  text_output_printf("HBA IS: 0b%b\n", ahci_data->hba->interrupt_status);

  // TODO: Only loop over ports with bits set in hba->interrupt_status
  for (size_t i = 0; i < ahci_data->num_devices; ++i) {
    AHCIDevice *device = &ahci_data->devices[i];
    HBAPort *port = port_from_device(device);

    if (port->interrupt_status > 0) {
      semaphore_up(&device->pending_command, 1);
      // Clear port interrupt status
      port->interrupt_status = ALL_ONES;
    }
  }

  // Clear HBA interrupt status when we are done
  ahci_data->hba->interrupt_status = ALL_ONES;
}

void ahci_register() {
  REQUIRE_MODULE("pci");

  PCIDeviceDriver interface = {
    .class_code = PCI_MASS_STORAGE_CLASS_CODE,
    .subclass = PCI_SATA_SUBCLASS,
    .program_if = PCI_AHCI_V1_PROGRAM_IF,

    .init = ahci_driver_init,
    .execute_command = ahci_execute_command,
    .isr = ahci_isr
  };

  pci_register_device_driver(interface);
}
