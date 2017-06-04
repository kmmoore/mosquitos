#include <kernel/kernel_common.h>

#ifndef _PCI_DEVICE_DRIVER_INTERFACE_H
#define _PCI_DEVICE_DRIVER_INTERFACE_H

typedef struct _PCIDevice PCIDevice;
typedef struct _PCIDeviceDriver PCIDeviceDriver;

typedef enum {
  PCI_ERROR_NONE = 0,
  PCI_ERROR_COMMAND_NOT_IMPLEMENTED,
  PCI_ERROR_INVALID_PARAMETERS,
  PCI_ERROR_INPUT_BUFFER_TOO_SMALL,
  PCI_ERROR_OUTPUT_BUFFER_TOO_SMALL,
  PCI_ERROR_DEVICE_ERROR
} PCIDeviceDriverError;

typedef void (*PCIDeviceDriverInitFunction)(PCIDeviceDriver *driver);

typedef PCIDeviceDriverError (*PCIDeviceDriverCommandFunction)(
    PCIDeviceDriver *driver, uint64_t command_id, const void *input_buffer,
    uint64_t input_buffer_size, void *output_buffer,
    uint64_t output_buffer_size);

typedef void (*PCIDeviceDriverISR)(PCIDeviceDriver *driver);

struct _PCIDeviceDriver {
  int class_code, subclass, program_if;

  const char *driver_name;

  PCIDeviceDriverInitFunction init;
  PCIDeviceDriverCommandFunction execute_command;
  PCIDeviceDriverISR isr;

  PCIDevice *device;
  void *driver_data;
};

#endif
