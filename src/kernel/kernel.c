#include <kernel/kernel.h>

#include <efi.h>
#include <efilib.h>

#include <common/mem_util.h>
#include <kernel/util.h>

#include <kernel/module_manager.h>

#include <kernel/drivers/graphics.h>
#include <kernel/drivers/text_output.h>
#include <kernel/drivers/keyboard_controller.h>
#include <kernel/drivers/pci.h>
#include <kernel/drivers/ahci.h>

#include <kernel/drivers/acpi.h>
#include <kernel/drivers/interrupt.h>
#include <kernel/drivers/exception.h>
#include <kernel/drivers/timer.h>
#include <kernel/drivers/serial_port.h>

#include <kernel/memory/virtual_memory.h>
#include <kernel/memory/kmalloc.h>

#include <kernel/threading/scheduler.h>
#include <kernel/threading/mutex/semaphore.h>

#include <common/build_info.h>

void * kernel_main_thread();

// Pre-threaded initialization is done here
void kernel_main(KernelInfo info) {
  // Disable interrupts as we have no way to handle them now
  cli();

  module_manager_init();

  serial_port_init();

  graphics_init(info.gop);
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
  ahci_init();

  text_output_set_foreground_color(0x0000FF00);
  text_output_printf("\nKernel initialization complete.\n\n");
  text_output_set_foreground_color(0x00FFFFFF);

  thread_exit(); // TODO: Maybe make returning do the same thing?
  return NULL;
}
