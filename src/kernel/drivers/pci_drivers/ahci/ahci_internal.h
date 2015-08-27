#include <kernel/kernel_common.h>
#include <kernel/drivers/pci_drivers/ahci/ahci_types.h>
#include <kernel/drivers/pci_drivers/pci_device_driver.h>

#ifndef _AHCI_H
#define _AHCI_H

#define ATAPI_COMMAND_LENGTH 12

typedef struct _AHCIDevice AHCIDevice;

enum AHCIDeviceType ahci_device_type(AHCIDevice *device);
struct AHCIDeviceInfo * ahci_device_info(AHCIDevice *device);

void ahci_clear_pending_interrupts(AHCIDevice *device);
FISRegisterH2D * ahci_initialize_command_fis(AHCIDevice *device, int slot, bool write,
                                             bool prefetchable, uint64_t byte_size,
                                             uint8_t *dma_buffer, bool atapi,
                                             uint8_t *atapi_command);
void ahci_set_command_fis_lba(FISRegisterH2D *command_fis, uint64_t address, uint64_t block_count);

int ahci_begin_command(AHCIDevice *device);
bool ahci_issue_command(AHCIDevice *device, int slot);

void ahci_copy_identify_string(uint16_t *identify_buffer, uint8_t offset, uint8_t word_length, 
                               uint8_t *destination);
#endif
