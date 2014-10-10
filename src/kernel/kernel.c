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

void * kernel_main_thread();

// Pre-threaded initialization is done here
void kernel_main(KernelInfo info) {

  cli();

  module_manager_init();

  serial_port_init();

  text_output_init(info.gop);
  text_output_set_background_color(0x00000000);

  text_output_clear_screen();

  text_output_set_foreground_color(0x0000FF00);
  text_output_printf("MosquitOS -- A tiny, annoying operating system\n");
  text_output_set_foreground_color(0x00FFFF00);
  text_output_printf("Built from %s on %s\n\n", build_git_info, build_time);
  text_output_set_foreground_color(0x00FFFFFF);

  // Initialize subsystems
  acpi_init(info.xdsp_address);
  interrupt_init();
  exception_init();

  // Now that interrupt/exception handlers are set up, we can enable interrupts
  sti();

  // Set up the dynamic memory subsystem
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

// Initialization that needs a threaded context is done here
void * kernel_main_thread() {
  // Full acpica needs dynamic memory and scheduling
  acpi_enable_acpica();

  // PCI needs APCICA to determine IRQ mappings
  pci_init();
  sata_init();
  
  // kmalloc_print_free_list();

  text_output_set_foreground_color(0x0000FF00);
  text_output_printf("\nKernel initialization complete.\n\n");

  thread_exit(); // TODO: Make returning do the same thing;
  return NULL;
}
