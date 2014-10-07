#include "kernel_common.h"

#include <efi.h>
#include <efilib.h>

#include "kernel.h"
#include "../common/mem_util.h"
#include "util.h"

#include "drivers/text_output.h"
#include "drivers/keyboard_controller.h"
#include "drivers/pci.h"
#include "drivers/sata.h"

#include "hardware/acpi.h"
#include "hardware/interrupt.h"
#include "hardware/exception.h"
#include "hardware/timer.h"
#include "hardware/serial_port.h"

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

  serial_port_init();

  text_output_init(info.gop);

  text_output_clear_screen(0x00000000);
  text_output_printf("MosquitOS -- A tiny, annoying operating system\n");
  text_output_printf("Built from %s on %s\n\n", build_git_info, build_time);

  // Initialize subsystems
  // TODO: Break these into initialization files
  acpi_init(info.xdsp_address);
  interrupt_init();
  exceptions_init();

  vm_init(info.memory_map, info.mem_map_size, info.mem_map_descriptor_size);

  // Now that interrupt/exception handlers are set up, we can enable interrupts
  sti();
  // sata_init();

  timer_init();
  keyboard_controller_init();

  // Set up scheduler
  scheduler_init();

  KernelThread *main_thread = thread_create(kernel_main_thread, NULL, 31, 2);
  thread_start(main_thread);

  scheduler_start_scheduling(); // kernel_main will not execute any more after this call

  assert(false); // We should never get here
}

void * DisplayOneDevice (ACPI_HANDLE ObjHandle, UINT32 Level, void *Context) {
  (void)ObjHandle, (void)Level, (void)Context;
  text_output_printf("DisplayOneDevice() called\n");
  // ACPI_STATUS Status;
  // ACPI_DEVICE_INFO Info;
  // ACPI_BUFFER Path;

  // char Buffer[256];

  // Path.Length = sizeof (Buffer);
  // Path.Pointer = Buffer;

  // /* Get the full path of this device and print it */
  // Status = AcpiHandleToPathname (ObjHandle, &Path);
  // if (ACPI_SUCCESS (Status)) {
  //   text_output_printf ("%s\n", Path.Pointer);
  // }
 
  // /* Get the device info for this device and print it */
  // Status = AcpiGetDeviceInfo (ObjHandle, &Info);
  // if (ACPI_SUCCESS (Status)) {
  //   text_output_printf (" HID: %.8X, ADR: %.8X, Status: %x\n", Info.HardwareId, Info.Address, Info.CurrentStatus);
  // }

  return NULL;
}

ACPI_STATUS desc_callback(ACPI_HANDLE Object, UINT32 NestingLevel UNUSED, void *Context UNUSED, void **ReturnValue UNUSED) {

  char name[128];
  ACPI_BUFFER name_buffer = { .Length = sizeof(name), .Pointer = &name };
  AcpiGetName(Object, ACPI_FULL_PATHNAME, &name_buffer);

  text_output_printf("%s\n", name);

  return AE_OK;
}

// Initialization that needs a threaded context is done here
void * kernel_main_thread() {
  pci_init();

  ACPI_STATUS ret;

  if (ACPI_FAILURE(ret = AcpiInitializeSubsystem())) {
    text_output_printf("ACPI INIT failure %s\n", AcpiFormatException(ret));
  }
  // text_output_printf("1\n");

  ACPI_TABLE_DESC tables[16];
  if (ACPI_FAILURE(ret = AcpiInitializeTables(tables, 16, FALSE))) {
    text_output_printf("ACPI INIT tables failure %s\n", AcpiFormatException(ret));
  }
  // text_output_printf("2\n");

    // vm_pmap(0xFEC00000, 1);
  if (ACPI_FAILURE(ret = AcpiLoadTables())) {
    text_output_printf("ACPI load tables failure %s\n", AcpiFormatException(ret));
  }
  // text_output_printf("3\n");

  if (ACPI_FAILURE(ret = AcpiEnableSubsystem(ACPI_FULL_INITIALIZATION))) {
    text_output_printf("ACPI enable failure %s\n", AcpiFormatException(ret));
  }
  text_output_printf("4\n");

  // cli();
  if (ACPI_FAILURE(ret = AcpiInitializeObjects(ACPI_FULL_INITIALIZATION))) {
    text_output_printf("ACPI init objects failure %s\n", AcpiFormatException(ret));
  }
  // sti();
  text_output_printf("5\n");

  ACPI_HANDLE system_bus_handle;
  ACPI_STATUS status = AcpiGetHandle(NULL, "\\_SB", &system_bus_handle);
  text_output_printf("Status ok? %d\n", status == AE_OK);

  void *walk_return_value;
  status = AcpiWalkNamespace(ACPI_TYPE_DEVICE, system_bus_handle, 10, desc_callback, NULL, NULL, &walk_return_value);
  text_output_printf("Status ok? %d\n", status == AE_OK);


  // ACPI_PCI_ROUTING_TABLE routing_table[128];
  // ACPI_BUFFER buffer;
  // buffer.Length = sizeof(routing_table);
  // buffer.Pointer = &routing_table;
  // AcpiGetIrqRoutingTable(


  thread_exit();
  return NULL;
}
