#include <kernel/kernel_common.h>

#ifndef _PCI_DEVICE_DRIVER_INTERFACE_H
#define _PCI_DEVICE_DRIVER_INTERFACE_H

typedef struct _PCIDevice PCIDevice;
typedef struct _PCIDeviceDriverInterface PCIDeviceDriverInterface;

typedef enum {
  PCI_ERROR_NONE = 0,
  PCI_ERROR_COMMAND_NOT_IMPLEMENTED,
  PCI_ERROR_INVALID_PARAMETERS,
  PCI_ERROR_INPUT_BUFFER_TOO_SMALL,
  PCI_ERROR_OUTPUT_BUFFER_TOO_SMALL,
  PCI_ERROR_DEVICE_ERROR
} PCIDeviceDriverInterfaceError;

typedef void (* PCIDeviceDriverInitFunction) (PCIDeviceDriverInterface *driver);

typedef PCIDeviceDriverInterfaceError (* PCIDeviceDriverCommandFunction)
        (PCIDeviceDriverInterface *driver, uint64_t command_id, void *input_buffer,
         uint64_t input_buffer_size, void *output_buffer, uint64_t output_buffer_size);

struct _PCIDeviceDriverInterface {
  int class_code, subclass, program_if;

  PCIDeviceDriverInitFunction init;
  PCIDeviceDriverCommandFunction execute_command;

  PCIDevice *device;
  void *driver_data;
};

#endif