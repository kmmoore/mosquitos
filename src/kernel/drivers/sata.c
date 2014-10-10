#include "sata.h"
#include "sata_types.h"
#include "text_output.h"
#include "pci.h"
#include "../util.h"
#include "timer.h"

#define MAX_CACHED_DEVICES 8

#define PCI_MASS_STORAGE_CLASS_CODE 0x01
#define PCI_SATA_SUBCLASS 0x06
#define PCI_AHCI_V1_PROGRAM_IF 0x01

#define SATA_SIG_ATA  0x00000101  // SATA drive
#define SATA_SIG_ATAPI  0xEB140101  // SATAPI drive
#define SATA_SIG_SEMB 0xC33C0101  // Enclosure management bridge
#define SATA_SIG_PM 0x96690101  // Port multiplier

static AHCIDeviceType device_type_in_port(HBAPort *port);

typedef struct {
  HBAMemory *hba;
  uint8_t port_number;
  AHCIDeviceType type;
} AHCIDevice;

struct {
  AHCIDevice devices[MAX_CACHED_DEVICES];
  uint8_t num_devices;
} sata_data;

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

  HBAPort *port = &device->hba->ports[device->port_number];
  uintptr_t clb = ((uintptr_t)port->command_list_base_address_upper << 32) | port->command_list_base_address;
  text_output_printf("Command list base address: %p\n", clb);
}

bool reset_hba(HBAMemory *hba) {
  hba->ghc &= (1 << 0); // Reset HBA
  timer_thread_sleep(10);
  hba->ghc &= (1 << 31); // Enable ACHI mode
  timer_thread_sleep(10); 

  return (hba->ghc & 0x01) == 0;
}

static void discover_devices(HBAMemory *hba) {
  // Check all implemented ports
  uint32_t ports_implemented = hba->ports_implemented;
  for (int i = 0; i < 32 && ports_implemented > 0; ++i) {
    if ((ports_implemented & (1 << i)) > 0) {
      AHCIDeviceType device_type = device_type_in_port(&hba->ports[i]);

      // Keep track of all ports with devices in them
      if (device_type != AHCI_DEVICE_NONE) {
        assert(sata_data.num_devices < MAX_CACHED_DEVICES);
        AHCIDevice *new_device = &sata_data.devices[sata_data.num_devices++];

        new_device->hba = hba;
        new_device->port_number = i;
        new_device->type = device_type;
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

  // We only know how to deal with single-function general devices
  assert(hba_device->multifunction == false);
  assert(hba_device->header_type == 0x0);

  uint32_t abar_word = pci_config_read_word(hba_device->bus, hba_device->slot, hba_device->function, 0x24);
  uintptr_t hba_base_address = (abar_word & FIELD_MASK(19, 13));
  HBAMemory *hba = (HBAMemory *)hba_base_address;

  uint16_t major_version = field_in_word(hba->version, 2, 2);
  uint16_t minor_version = field_in_word(hba->version, 1, 1);
  uint16_t subminor_version = field_in_word(hba->version, 1, 0);
  text_output_printf("AHCI Version: %d.%d.%d\n", major_version, minor_version, subminor_version);

  assert(major_version == 1 && minor_version <= 3); // We only know how to deal with ACHI version 1.0 - 1.3

  text_output_printf("HBA Capabilities: 0b%b\n", hba->capabilities);

  assert(reset_hba(hba));

  discover_devices(hba);

  for (int i = 0; i < sata_data.num_devices; ++i) {
    print_ahci_device(&sata_data.devices[i]);
  }

  return true;
}

void sata_init() {
  sata_data.num_devices = 0;

  // Find a SATA AHCI v1 PCI device
  // TODO: Handle multiple AHCI devices
  PCIDevice *hba = pci_find_device(PCI_MASS_STORAGE_CLASS_CODE, PCI_SATA_SUBCLASS, PCI_AHCI_V1_PROGRAM_IF);

  if (!hba) {
    panic("Could not find SATA AHCI PCI device!\n");
  }

  if (!initialize_hba(hba)) {
    panic("Could not initialize HBA!\n");
  }
}
