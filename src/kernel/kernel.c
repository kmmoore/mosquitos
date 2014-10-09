#include "kernel_common.h"

#include <efi.h>
#include <efilib.h>

#include "kernel.h"
#include "../common/mem_util.h"
#include "util.h"

#include "module_manager.h"

#include "drivers/text_output.h"
#include "drivers/keyboard_controller.h"
#include "drivers/pci.h"
#include "drivers/sata.h"

#include "drivers/acpi.h"
#include "drivers/interrupt.h"
#include "drivers/exception.h"
#include "drivers/timer.h"
#include "drivers/serial_port.h"

#include "memory/virtual_memory.h"
#include "memory/kmalloc.h"

#include "threading/scheduler.h"
#include "threading/mutex/semaphore.h"

#include "../common/build_info.h"

#include <acpi.h>

void * kernel_main_thread();

// Pre-threaded initialization is done here
void kernel_main(KernelInfo info) {

  cli();

  module_manager_init();

  serial_port_init();
  text_output_init(info.gop);

  text_output_clear_screen(0x00000000);
  text_output_printf("MosquitOS -- A tiny, annoying operating system\n");
  text_output_printf("Built from %s on %s\n\n", build_git_info, build_time);

  // Initialize subsystems
  // TODO: Break these into initialization files
  acpi_init(info.xdsp_address);
  interrupt_init();
  exception_init();

  // Now that interrupt/exception handlers are set up, we can enable interrupts
  sti();
  
  vm_init(info.memory_map, info.mem_map_size, info.mem_map_descriptor_size);

  timer_init();
  keyboard_controller_init();

  // Set up scheduler
  scheduler_init();

  KernelThread *main_thread = thread_create(kernel_main_thread, NULL, 31, 2);
  thread_start(main_thread);

  scheduler_start_scheduling(); // kernel_main will not execute any more after this call

  assert(false); // We should never get here
}

ACPI_STATUS desc_callback(ACPI_HANDLE Object, UINT32 NestingLevel UNUSED, void *Context UNUSED, void **ReturnValue UNUSED) {

  char name[128];
  ACPI_BUFFER name_buffer = { .Length = sizeof(name), .Pointer = &name };
  AcpiGetName(Object, ACPI_FULL_PATHNAME, &name_buffer);

  // ACPI_PCI_ROUTING_TABLE *table = (ACPI_PCI_ROUTING_TABLE *)Context;

  ACPI_DEVICE_INFO *device_info;
  assert(AcpiGetObjectInfo(Object, &device_info) == AE_OK);
  text_output_printf("%s\n", name);

  if (device_info->Flags == ACPI_PCI_ROOT_BRIDGE) { 
    // TODO figure out how to deal with multiple buses
    text_output_printf("Found PCI root bridge.\n");

    // ACPI_PCI_ROUTING_TABLE routing_table[128];
    //   ACPI_BUFFER buffer;
    //   buffer.Length = sizeof(routing_table);
    //   buffer.Pointer = &routing_table;
    //   status = AcpiGetIrqRoutingTable(system_bus_handle, &buffer);
    //   text_output_printf("Status ok? %d\n", status == AE_OK);
    //   assert(routing_table[0].Length == sizeof(ACPI_PCI_ROUTING_TABLE));

    //   for (int i = 0; i < 128; ++i) {
    //     if (routing_table[i].Length == 0) break;

    //     uint16_t device_number = routing_table[i].Address >> 16;

    //     text_output_printf("IRQ Pin %d, PCI device number: %d, Real IRQ: %d Source: %x %x %x %x\n", routing_table[i].Pin, device_number, routing_table[i].SourceIndex, routing_table[i].Source[0], routing_table[i].Source[1], routing_table[i].Source[2], routing_table[i].Source[3]);
    //   }
  }

  // text_output_printf("%s: _ADR: %llx, SUB: %s\n", name, device_info->Address, device_info->SubsystemId.String);

  ACPI_FREE(device_info);

  return AE_OK;
}

// Initialization that needs a threaded context is done here
void * kernel_main_thread() {
  acpi_enable_acpica();
  pci_init();

  ACPI_HANDLE system_bus_handle;
  ACPI_STATUS status = AcpiGetHandle(NULL, "\\_SB", &system_bus_handle);
  assert(status == AE_OK);

  void *walk_return_value;
  status = AcpiWalkNamespace(ACPI_TYPE_DEVICE, system_bus_handle, 1, desc_callback, NULL, NULL, &walk_return_value);
  assert(status == AE_OK);



  thread_exit();
  return NULL;
}
