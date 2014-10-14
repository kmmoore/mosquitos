#include "sata.h"
#include "sata_types.h"
#include "text_output.h"
#include "pci.h"
#include "interrupt.h"
#include "apic.h"
#include "../util.h"
#include "timer.h"
#include "../../common/mem_util.h"

// TOOD: Make sure we don't use PCI/SATA MMIO/DMA space for other stuff

#define SATA_IV 39

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

#define ATA_DEV_BUSY 0x80
#define ATA_DEV_DRQ 0x08

void sata_isr() {
  text_output_printf("SATA interrupt!\n");
  apic_send_eoi();
}
 
bool read(HBAPort *port, uint32_t startl, uint32_t starth, uint32_t count, uint8_t *buf) {
  port->interrupt_status = 0;   // Clear pending interrupt bits
  int spin = 0; // Spin lock timeout counter
  int slot = ahci_find_command_slot(port);
  if (slot == -1)
    return FALSE;
 
  HBACommandHeader *cmdheader = (HBACommandHeader*)(uintptr_t)port->command_list_base_address;
  cmdheader += slot;
  cmdheader->cfl = sizeof(FISRegisterH2D)/sizeof(uint32_t); // Command FIS size in dwords
  cmdheader->w = 0;   // Read from device
  cmdheader->p = 1;
  cmdheader->prdtl = (uint16_t)((count-1)>>4) + 1;  // PRDT entries count
 
  HBACommandTable *cmdtbl = (HBACommandTable*)(uintptr_t)(cmdheader->ctba);
  memset(cmdtbl, 0, sizeof(HBACommandTable) +
    (cmdheader->prdtl-1)*sizeof(HBAPRDTEntry));
 
  // 8K bytes (16 sectors) per PRDT
  for (int i=0; i<cmdheader->prdtl-1; i++) {
    cmdtbl->prdt_entry[i].dba = field_in_word((uint64_t)buf, 0, 4);
    cmdtbl->prdt_entry[i].dbau = field_in_word((uint64_t)buf, 4, 4);
    cmdtbl->prdt_entry[i].dbc = 8*1024; // 8K bytes
    cmdtbl->prdt_entry[i].i = 1;
    buf += 4*1024;  // 4K words
    count -= 16;  // 16 sectors
  }
  // Last entry
  cmdtbl->prdt_entry[cmdheader->prdtl-1].dba = field_in_word((uint64_t)buf, 0, 4);
  cmdtbl->prdt_entry[cmdheader->prdtl-1].dbau = field_in_word((uint64_t)buf, 4, 4);
  cmdtbl->prdt_entry[cmdheader->prdtl-1].dbc = count<<9; // 512 bytes per sector
  cmdtbl->prdt_entry[cmdheader->prdtl-1].i = 1;
 
  // Setup command
  FISRegisterH2D *cmdfis = (FISRegisterH2D*)(uintptr_t)(&cmdtbl->cfis);
 
  cmdfis->fis_type = FIS_TYPE_REG_H2D;
  cmdfis->c = 1;  // Command
  cmdfis->command = 0x25;//ATA_CMD_READ_DMA_EX;
 
  cmdfis->lba0 = (uint8_t)startl;
  cmdfis->lba1 = (uint8_t)(startl>>8);
  cmdfis->lba2 = (uint8_t)(startl>>16);
  cmdfis->device = 1<<6;  // LBA mode
 
  cmdfis->lba3 = (uint8_t)(startl>>24);
  cmdfis->lba4 = (uint8_t)starth;
  cmdfis->lba5 = (uint8_t)(starth>>8);
 
  cmdfis->countl = (count & 0xFF);
  cmdfis->counth = (count >> 8) & 0xFF;
 
  // The below loop waits until the port is no longer busy before issuing a new command
  while ((port->tfd & (ATA_DEV_BUSY | ATA_DEV_DRQ)) && spin < 1000000)
  {
    spin++;
  }
  if (spin == 1000000)
  {
    text_output_printf("Port is hung\n");
    return false;
  }
 
  text_output_printf("is: 0b%b\n", port->interrupt_status);
 
  port->command_issue = 1 << slot; // Issue command
 
  // // Wait for completion
  // while (1)
  // {
  //   // In some longer duration reads, it may be helpful to spin on the DPS bit 
  //   // in the PxIS port field as well (1 << 5)
  //   if ((port->command_issue & (1 << slot)) == 0) 
  //     break;
  //   if (port->interrupt_status & (1 << 30)) // Task file error
  //   {
  //     text_output_printf("Read disk error 0b%b\n", port->interrupt_status);
  //     return false;
  //   }
  // }
 
  // // Check again
  // if (port->interrupt_status & (1 << 30)) // HBA_PxIS_TFES
  // {
  //   text_output_printf("Read disk error\n");
  //   return false;
  // }
 
  return true;
}

void ahci_execute_command(AHCIDevice *device) {
  HBAPort *port = &device->hba->ports[device->port_number];

  uintptr_t clb = ((uintptr_t)port->command_list_base_address_upper << 32) | port->command_list_base_address;
  text_output_printf("Command list base address: %p\n", clb);

  uintptr_t fb = ((uintptr_t)port->fbu << 32) | port->fb;
  text_output_printf("FIS base address: %p\n", fb);

  port->command |= (1 << 4); // Enable FIS receiving
  port->command |= (1 << 0); // Start port
  port->interrupt_enable = ALL_ONES;

  text_output_printf("IE: 0b%b\n", port->interrupt_enable);
  text_output_printf("CMD: 0b%b\n", port->command);

  // text_output_printf("\nHard drive read:\n\n");
  // uint32_t old_fg_color = text_output_get_foreground_color();
  // text_output_set_foreground_color(0x00FFFF00);

  uint8_t buffer[512];
  if(read(port, 0, 0, 1, buffer)) {
  }

  timer_thread_sleep(100);

  text_output_printf("is: 0b%b\n", port->interrupt_status);

  // text_output_set_foreground_color(old_fg_color);

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
    ahci_execute_command(device);
  }

}

bool reset_hba(HBAMemory *hba) {
  hba->ghc |= (1 << 0); // Reset HBA
  while (hba->ghc & 0x01) __asm__ volatile ("nop"); // Wait for reset

  hba->ghc |= (1 << 31); // Enable ACHI mode
  hba->ghc |= (1 << 1); // Set interrupt enable

  text_output_printf("HBA GHC: 0b%b\n", hba->ghc);


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

  text_output_printf("HBA Capabilities: 64-bit? %d, SSS? %d, AHCI Only? %d, num cmd slots: %d\n", (hba->capabilities & (1 << 31)) > 0, (hba->capabilities & (1 << 27)) > 0, (hba->capabilities & (1 << 18)) > 0, ((hba->capabilities & FIELD_MASK(5, 8)) >> 8) + 1);

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
