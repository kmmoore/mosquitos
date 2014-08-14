#include <efi.h>
#include <efilib.h>

#include <acpi.h>

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

#include "memory/virtual_memory.h"
#include "memory/kmalloc.h"

#include "threading/scheduler.h"

#include "threading/mutex/semaphore.h"

#include "../common/build_info.h"

void * thread2_main(void *p) {
  text_output_printf("[2] Started!\n");
  Semaphore *sema = (Semaphore *)p;
  bool got_semaphore = semaphore_down(sema, 1, -1);
  text_output_printf("[2] Sema value: %d, got semaphore: %d\n", semaphore_value(sema), got_semaphore);
  text_output_printf("[2] Exiting!\n");
  thread_exit();
  return NULL;
}

void * thread1_main(void *p) {
  Semaphore *sema = (Semaphore *)p;
  text_output_printf("[1] Started!\n");
  text_output_printf("[1] Spawning thread 2...\n");
  timer_thread_stall(100);

  KernelThread *t2 = thread_create(thread2_main, NULL, 31, 2);
  thread_start(t2);

  timer_thread_sleep(1000);
  text_output_printf("[1] Woke up\n");
  semaphore_up(sema, 1);

  thread_exit();
  return NULL;
}

void kernel_main(KernelInfo info) {

  cli();

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

  pci_init();
  // sata_init();

  timer_init();
  keyboard_controller_init();

  // ACPI_STATUS ret;

  // if (ACPI_FAILURE(ret = AcpiInitializeSubsystem())) {
  //   text_output_printf("ACPI INIT failure %s\n", AcpiFormatException(ret));
  // }

  // // if (ACPI_FAILURE(ret = AcpiInitializeTables(NULL, 0, FALSE))) {
  // //   text_output_printf("ACPI INIT tables failure %s\n", AcpiFormatException(ret));
  // // }

  // // vm_pmap(0xFEC00000, 1);
  // // if (ACPI_FAILURE(ret = AcpiLoadTables())) {
  // //   text_output_printf("ACPI load tables failure %s\n", AcpiFormatException(ret));
  // // }

  // // if (ACPI_FAILURE(ret = AcpiEnableSubsystem(ACPI_FULL_INITIALIZATION))) {
  // //   text_output_printf("ACPI enable failure %s\n", AcpiFormatException(ret));
  // // }

  // // if (ACPI_FAILURE(ret = AcpiInitializeObjects(ACPI_FULL_INITIALIZATION))) {
  // //   text_output_printf("ACPI init objects failure %s\n", AcpiFormatException(ret));
  // // }

  // Set up scheduler
  scheduler_init();

  Semaphore sema;
  semaphore_init(&sema, 0);

  KernelThread *t1 = thread_create(thread1_main, &sema, 31, 2);
  thread_start(t1);

  scheduler_start_scheduling(); // kernel_main will not execute any more after this call
  // while(1);

  assert(false); // We should never get here
}