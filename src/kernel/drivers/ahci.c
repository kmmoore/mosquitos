#include <kernel/drivers/ahci.h>
#include <kernel/drivers/ahci_types.h>
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

#define SATA_SIG_ATA    0x00000101  // SATA drive
#define SATA_SIG_ATAPI  0xEB140101  // SATAPI drive
#define SATA_SIG_SEMB   0xC33C0101  // Enclosure management bridge
#define SATA_SIG_PM     0x96690101  // Port multiplier

#define ATA_DEV_BUSY (1 << 7)
#define ATA_DEV_DRQ  (1 << 3)

#define ATA_CMD_READ_DMA_EX  0x25
#define ATA_CMD_WRITE_DMA_EX 0x35
#define ATA_CMD_IDENTIFY     0xEC

#define MAX_BYTES_PER_PRDT (1 << 22)
#define BYTES_PER_SECTOR   512

static AHCIDeviceType device_type_in_port(HBAPort *port);

typedef struct _AHCIDevice {
  HBAMemory *hba;
  uint8_t port_number;
  AHCIDeviceType type;
  Semaphore pending_command;
} AHCIDevice;

struct {
  // TODO: Store HBA(s?) capabilities
  bool use_64_bits;
  AHCIDevice devices[MAX_CACHED_DEVICES];
  uint8_t num_devices;
} sata_data;

HBAPort * port_from_device(AHCIDevice *device) {
  assert(device);
  assert(device->hba);
  assert(device->hba->ports);
  return &device->hba->ports[device->port_number];
}

// Find a free command list slot
int ahci_find_command_slot(HBAPort *port) {
  // If not set in SACT and CI, the slot is free
  uint32_t slots = (port->sata_active | port->command_issue);
  for (int i = 0; i < 32; i++) {
    if ((slots & (1 << i)) == 0) {
      return i;
    }
  }

  text_output_printf("Cannot find free command list entry\n");
  return -1;
}

// TODO: This ISR might be shared with other PCI devices,
// it should be handled by the PCI driver and then passed
// to the AHCI driver if necessary.
void sata_isr() {
  for (size_t i = 0; i < sata_data.num_devices; ++i) {
    AHCIDevice *device = &sata_data.devices[i];
    HBAPort *port = port_from_device(device);

    if (port->interrupt_status > 0) {
      semaphore_up(&device->pending_command, 1);
    }
  }

  apic_send_eoi();
}

void port_start(HBAPort *port) {
  // Set up the HBA to process a command (AHCI Spec. p.26)
  port->command |= (1 << 4); // Enable FIS receiving
  port->command |= (1 << 0); // Start port
  port->interrupt_enable = ALL_ONES; // Enable all interrupts

  assert((port->command & (1 << 4)) != 0);
  assert((port->command & (1 << 0)) != 0);
}

void port_stop(HBAPort *port) {
  // Disable command processing
  port->command &= ~(1 << 0); // Stop port
  assert((port->command & (1 << 0)) == 0);
}

FISRegisterH2D * new_command_fis(AHCIDevice *device, int slot, bool write, bool prefetchable, uint64_t byte_size, uint8_t *dma_buffer) {
  HBAPort *port = port_from_device(device);

  // Get the command header associated with our free slot
  // TODO: Refactor this into a #define or something
  uintptr_t command_header_address = (uintptr_t)port->command_list_base_address;
  if (sata_data.use_64_bits) {
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
  if (sata_data.use_64_bits) {
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
bool issue_command(AHCIDevice *device, int slot) {
  HBAPort *port = port_from_device(device);

  // Wait until the port is free
  int spin = 0;
  while ((port->tfd & (ATA_DEV_BUSY | ATA_DEV_DRQ)) && spin < 1000000) {
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
    semaphore_down(&device->pending_command, 1, 0); // Sleep until we get an interrupt
  }

  return true;
}

bool sata_read(AHCIDevice *device, uint64_t lba, uint32_t count, uint8_t *buffer) {
  // We only know how to read from SATA devices
  assert(device->type == AHCI_DEVICE_SATA);

  HBAPort *port = port_from_device(device);

  port_start(port);

  port->interrupt_status = 0;   // Clear pending interrupt bits
  int slot = ahci_find_command_slot(port);
  if (slot == -1) {
    return FALSE;
  }

  uint64_t requested_bytes = count * BYTES_PER_SECTOR;
 
  // Setup command
  FISRegisterH2D *command_fis = new_command_fis(device, slot, false, true, requested_bytes, buffer);
 
  command_fis->command = ATA_CMD_READ_DMA_EX;
 
  command_fis->lba0 = field_in_word(lba, 0, 1);
  command_fis->lba1 = field_in_word(lba, 1, 1);
  command_fis->lba2 = field_in_word(lba, 2, 1);
  command_fis->device = 1 << 6;  // LBA mode;
 
  command_fis->lba3 = field_in_word(lba, 3, 1);
  command_fis->lba4 = field_in_word(lba, 4, 1);
  command_fis->lba5 = field_in_word(lba, 5, 1);
 
  command_fis->countl = field_in_word(count, 0, 1);
  command_fis->counth = field_in_word(count, 1, 1);
 
  bool success = issue_command(device, slot);

  // TODO: Make sure no one else is using the port
  port_stop(port);

  return success;
}

// buf should be 512 bytes
bool sata_identify(AHCIDevice *device, uint8_t *buffer) {
  HBAPort *port = port_from_device(device);

  port_start(port);

  port->interrupt_status = 0; // Clear pending interrupt bits
  int slot = ahci_find_command_slot(port);
  if (slot == -1) {
    return false;
  }
 
  // Setup command
  FISRegisterH2D *command_fis = new_command_fis(device, slot, false, true, 512, buffer);
  command_fis->command = ATA_CMD_IDENTIFY;
  command_fis->device = 0;
 
  bool success = issue_command(device, slot);

  // TODO: Make sure no one else is using the port
  port_stop(port);

  return success;
}

static void print_ahci_device(AHCIDevice *device) {
  char *type = "Unknown";
  switch (device->type) {
    case AHCI_DEVICE_NONE: type = "None"; break;
    case AHCI_DEVICE_SATA: type = "SATA"; break;
    case AHCI_DEVICE_SATAPI: type = "SATAPI"; break;
    case AHCI_DEVICE_SEMB: type = "SEMB"; break;
    case AHCI_DEVICE_PM: type = "Port Multiplier"; break;
    case AHCI_DEVICE_UNKNOWN: type = "Unknown"; break;
  }

  text_output_printf("[AHCI Device] Port #: %d, Type: %s\n", device->port_number, type);
  if (device->type == AHCI_DEVICE_SATA) {
    text_output_printf("Signature: 0b%b\n", field_in_word(port_from_device(device)->signature, 0, 1));
  }

}

bool reset_hba(HBAMemory *hba) {
  hba->ghc |= (1 << 0); // Reset HBA
  while (hba->ghc & 0x01) __asm__ volatile ("nop"); // Wait for reset

  hba->ghc |= (1 << 31); // Enable ACHI mode
  hba->ghc |= (1 << 1); // Set interrupt enable

  return (hba->ghc & 0x01) == 0;
}

static void discover_devices(HBAMemory *hba) {
  // Check all implemented ports
  uint32_t ports_implemented = hba->ports_implemented;
  for (int i = 0; i < 32; ++i) {
    if ((ports_implemented & (1 << i)) > 0) {
      AHCIDeviceType device_type = device_type_in_port(&hba->ports[i]);

      // Keep track of all ports with devices in them
      if (device_type != AHCI_DEVICE_NONE) {
        assert(sata_data.num_devices < MAX_CACHED_DEVICES);
        AHCIDevice *new_device = &sata_data.devices[sata_data.num_devices++];

        new_device->hba = hba;
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
    case SATA_SIG_ATAPI: return AHCI_DEVICE_SATAPI;
    case SATA_SIG_SEMB:  return AHCI_DEVICE_SEMB;
    case SATA_SIG_PM:    return AHCI_DEVICE_PM;
    case SATA_SIG_ATA:   return AHCI_DEVICE_SATA;
    default:             return AHCI_DEVICE_UNKNOWN;
  }
}

bool initialize_hba(PCIDevice *hba_device) {

  // We only know how to deal with general devices
  assert(hba_device->header_type == 0x0);

  // TODO: Fall back to polling if there isn't a PCI interrupt associated. In reality this should never(?) happen.
  assert(hba_device->has_interrupts);

  uint32_t cmd_word = PCI_HEADER_READ_FIELD_WORD(hba_device->bus, hba_device->slot, hba_device->function, command);
  text_output_printf("HBA PCI CMD Field: 0b%b\n", PCI_HEADER_FIELD_IN_WORD(cmd_word, command) & (1 << 10));

  text_output_printf("HBA Slot: %d, IRQ #: %d\n", hba_device->slot, hba_device->real_irq);

  // TODO: Try to set up MSI again
  interrupt_register_handler(SATA_IV, sata_isr);
  ioapic_map(hba_device->real_irq, SATA_IV);

  uint32_t abar_word = pci_config_read_word(hba_device->bus, hba_device->slot, hba_device->function, 0x24);
  uintptr_t hba_base_address = (abar_word & FIELD_MASK(19, 13));
  HBAMemory *hba = (HBAMemory *)hba_base_address;

  uint16_t major_version = field_in_word(hba->version, 2, 2);
  uint16_t minor_version = field_in_word(hba->version, 1, 1);
  uint16_t subminor_version = field_in_word(hba->version, 0, 1);
  text_output_printf("HBA AHCI Version: %d.%d.%d\n", major_version, minor_version, subminor_version);

  assert(major_version == 1 && minor_version <= 3); // We only know how to deal with ACHI version 1.0 - 1.3

  sata_data.use_64_bits = (hba->capabilities & (1 << 31)) > 0;

  text_output_printf("HBA Capabilities: 64-bit? %d, SSS? %d, AHCI Only? %d, num cmd slots: %d\n", (hba->capabilities & (1 << 31)) > 0, (hba->capabilities & (1 << 27)) > 0, (hba->capabilities & (1 << 18)) > 0, ((hba->capabilities & FIELD_MASK(5, 8)) >> 8) + 1);

  assert(reset_hba(hba));

  discover_devices(hba);

  for (int i = 0; i < sata_data.num_devices; ++i) {
    print_ahci_device(&sata_data.devices[i]);
  }

  return true;
}

void ahci_init() {
  REQUIRE_MODULE("pci");

  sata_data.num_devices = 0;

  // Find a SATA AHCI v1 PCI device
  // TODO: Handle multiple AHCI devices
  // TODO: The PCI driver should load a new AHCI driver for each HBA
  PCIDevice *hba = pci_find_device(PCI_MASS_STORAGE_CLASS_CODE, PCI_SATA_SUBCLASS, PCI_AHCI_V1_PROGRAM_IF);

  if (!hba) {
    panic("Could not find SATA AHCI PCI device!\n");
  }

  if (!initialize_hba(hba)) {
    panic("Could not initialize HBA!\n");
  }

  for (int i = 0; i < sata_data.num_devices; ++i) {
    if (sata_data.devices[i].type == AHCI_DEVICE_SATA) {
      uint16_t buffer[256];
      memset(buffer, 0, sizeof(buffer));
      if (sata_identify(&sata_data.devices[i], (uint8_t *)buffer)) {
        text_output_printf("Supports LBA48? %s\n", (buffer[83] & (1 << 10)) > 0 ? "yes" : "no");
        text_output_printf("Max LBA: 0x%.4x%.4x%.4x%.4x\n", buffer[103], buffer[102], buffer[101], buffer[100]);
        uint64_t byte_capacity = (buffer[100] + ((uint64_t)buffer[101] << 16) + ((uint64_t)buffer[101] << 32) + ((uint64_t)buffer[101] << 48)) * 512;

        text_output_printf("Capacity: %d bytes\n", byte_capacity);
      }
    }
  }

  REGISTER_MODULE("ahci");
}
