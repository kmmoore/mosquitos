#include <kernel/kernel.h>

#include <efi.h>
#include <efilib.h>

#include <common/mem_util.h>
#include <kernel/util.h>

#include <kernel/module_manager.h>

#include <kernel/drivers/graphics.h>
#include <kernel/drivers/keyboard_controller.h>
#include <kernel/drivers/pci.h>
#include <kernel/drivers/pci_drivers/ahci/ahci.h>
#include <kernel/drivers/text_output.h>

#include <kernel/drivers/filesystem.h>
#include <kernel/drivers/filesystem_tree.h>
#include <kernel/drivers/filesystems/mfs.h>

#include <kernel/drivers/acpi.h>
#include <kernel/drivers/apic.h>
#include <kernel/drivers/exception.h>
#include <kernel/drivers/gdt.h>
#include <kernel/drivers/interrupt.h>
#include <kernel/drivers/serial_port.h>
#include <kernel/drivers/timer.h>

#include <kernel/memory/kmalloc.h>
#include <kernel/memory/virtual_memory.h>

#include <kernel/threading/mutex/lock.h>
#include <kernel/threading/scheduler.h>

#include <common/build_info.h>

void *kernel_main_thread();

Lock kernel_lock;

// Pre-threaded initialization is done here
void kernel_main(KernelInfo info) {
  // Disable interrupts as we have no way to handle them now
  cli();

  module_manager_init();

  serial_port_init();

  graphics_init(info.gop);
  text_output_init();
  text_output_set_background_color(0x00000000);

  text_output_clear_screen();

  text_output_set_foreground_color(0x0000FF00);
  text_output_printf("MosquitOS -- A tiny, annoying operating system\n");
  text_output_set_foreground_color(0x00FFFF00);
  text_output_printf("Built from %s on %s\n\n", build_git_info, build_time);
  text_output_set_foreground_color(0x00FFFFFF);

  lock_init(&kernel_lock);

  // Initialize subsystems
  acpi_init(info.xdsp_address);
  gdt_init();
  apic_init();
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

  KernelThread *main_thread = thread_create(kernel_main_thread, NULL, 31, 4);
  thread_start(main_thread);

  // kernel_main will not execute any more after this call
  scheduler_start_scheduling();

  assert(false);  // We should never get here
}

void *keyboard_echo_thread() {
  lock_acquire(&kernel_lock, -1);
  text_output_printf("Starting keyboard_echo_thread.\n");
  lock_release(&kernel_lock);

  while (1) {
    int c = keyboard_controller_read_char(false);
    if (c >= 0) {
      if (c == '\b' || c == 127) {
        text_output_backspace();
      } else {
        uint32_t old_fg = text_output_get_foreground_color();
        text_output_set_foreground_color(0x006666FF);
        text_output_putchar(c);
        text_output_set_foreground_color(old_fg);
      }
    }
  }

  return NULL;
}

// Initialization that needs a threaded context is done here
void *kernel_main_thread() {
  // Full acpica needs dynamic memory and scheduling
  acpi_enable_acpica();

  // PCI needs APCICA to determine IRQ mappings
  pci_init();

  // Register PCI drivers
  ahci_register();

  // Once we've registered the PCI drivers, enumerate and instantiate PCI
  // drivers
  pci_enumerate_devices();

  // Set up low-priority thread to echo keyboard to screen
  KernelThread *keyboard_thread =
      thread_create(keyboard_echo_thread, NULL, 1, 1);
  thread_start(keyboard_thread);

  // Register filesystems
  filesystem_init();
  mfs_in_memory_register();
  mfs_sata_register();

  // Enumerate filesystems
  filesystem_tree_init();

  lock_acquire(&kernel_lock, -1);
  text_output_set_foreground_color(0x0000FF00);
  text_output_printf(
      "\nKernel initialization complete. Exiting kernel_main_thread.\n\n");
  text_output_set_foreground_color(0x00FFFFFF);
  lock_release(&kernel_lock);

  return NULL;
}
