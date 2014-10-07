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

  // ACPI_PCI_ROUTING_TABLE *table = (ACPI_PCI_ROUTING_TABLE *)Context;

  ACPI_DEVICE_INFO *device_info;
  assert(AcpiGetObjectInfo(Object, &device_info) == AE_OK);

  text_output_printf("%s: _ADR: %llx, SUB: %s\n", name, device_info->Address, device_info->SubsystemId.String);

  ACPI_FREE(device_info);

  return AE_OK;
}

// Initialization that needs a threaded context is done here
void * kernel_main_thread() {
  pci_init();

  ACPI_STATUS ret;

  if (ACPI_FAILURE(ret = AcpiInitializeSubsystem())) {
    text_output_printf("ACPI INIT failure %s\n", AcpiFormatException(ret));
  }

  ACPI_TABLE_DESC tables[16];
  if (ACPI_FAILURE(ret = AcpiInitializeTables(tables, 16, FALSE))) {
    text_output_printf("ACPI INIT tables failure %s\n", AcpiFormatException(ret));
  }

  if (ACPI_FAILURE(ret = AcpiLoadTables())) {
    text_output_printf("ACPI load tables failure %s\n", AcpiFormatException(ret));
  }

  if (ACPI_FAILURE(ret = AcpiEnableSubsystem(ACPI_FULL_INITIALIZATION))) {
    text_output_printf("ACPI enable failure %s\n", AcpiFormatException(ret));
  }

  // Enable IO APIC
  ACPI_OBJECT_LIST        Params;
  ACPI_OBJECT             Obj;

  Params.Count = 1;
  Params.Pointer = &Obj;
  
  Obj.Type = ACPI_TYPE_INTEGER;
  Obj.Integer.Value = 1;     // 0 = PIC, 1 = APIC

  ACPI_STATUS status = AcpiEvaluateObject(NULL, "\\_PIC", &Params, NULL);
  assert(status == AE_OK);

  if (ACPI_FAILURE(ret = AcpiInitializeObjects(ACPI_FULL_INITIALIZATION))) {
    text_output_printf("ACPI init objects failure %s\n", AcpiFormatException(ret));
  }

  ACPI_HANDLE system_bus_handle;
  status = AcpiGetHandle(NULL, "\\_SB.PCI0", &system_bus_handle);
  assert(status == AE_OK);

  ACPI_PCI_ROUTING_TABLE routing_table[128];
  ACPI_BUFFER buffer;
  buffer.Length = sizeof(routing_table);
  buffer.Pointer = &routing_table;
  status = AcpiGetIrqRoutingTable(system_bus_handle, &buffer);
  text_output_printf("Status ok? %d\n", status == AE_OK);
  assert(routing_table[0].Length == sizeof(ACPI_PCI_ROUTING_TABLE));

  for (int i = 0; i < 128; ++i) {
    if (routing_table[i].Length == 0) break;


    text_output_printf("IRQ Pin %d, PCI Address: %llx, SourceIndex: %x Source: %x %x %x %x\n", routing_table[i].Pin, routing_table[i].Address, routing_table[i].SourceIndex, routing_table[i].Source[0], routing_table[i].Source[1], routing_table[i].Source[2], routing_table[i].Source[3]);
  }

  void *walk_return_value;
  status = AcpiWalkNamespace(ACPI_TYPE_DEVICE, system_bus_handle, 100, desc_callback, NULL, NULL, &walk_return_value);
  assert(status == AE_OK);



  thread_exit();
  return NULL;
}
