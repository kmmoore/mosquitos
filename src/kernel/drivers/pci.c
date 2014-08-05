#include "pci.h"
#include "../util.h"
#include "text_output.h"

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
} PCIConfigurationHeader;

uint16_t pciConfigReadWord (uint8_t bus, uint8_t slot,
                             uint8_t func, uint8_t offset)
 {
    uint32_t address;
    uint32_t lbus  = (uint32_t)bus;
    uint32_t lslot = (uint32_t)slot;
    uint32_t lfunc = (uint32_t)func;
    uint16_t tmp = 0;
 
    /* create configuration address as per Figure 1 */
    address = (uint32_t)((lbus << 16) | (lslot << 11) |
              (lfunc << 8) | (offset & 0xfc) | ((uint32_t)0x80000000));
 
    /* write out the address */
    io_write_32(0xCF8, address);
    /* read in the data */
    /* (offset & 2) * 8) = 0 will choose the first word of the 32 bits register */
    tmp = (uint16_t)((io_read_32 (0xCFC) >> ((offset & 2) * 8)) & 0xffff);
    return (tmp);
 }

void pci_init() {
  text_output_printf("PCI: %d\n", pciConfigReadWord(0, 0, 0, 0));
}
