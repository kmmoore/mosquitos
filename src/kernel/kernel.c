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
#include <kernel/drivers/pci_drivers/ahci/ahci_driver.h>

#include <kernel/drivers/acpi.h>
#include <kernel/drivers/interrupt.h>
#include <kernel/drivers/exception.h>
#include <kernel/drivers/timer.h>
#include <kernel/drivers/serial_port.h>

#include <kernel/memory/virtual_memory.h>
#include <kernel/memory/kmalloc.h>

#include <kernel/threading/scheduler.h>
#include <kernel/threading/mutex/lock.h>

#include <common/build_info.h>

void * kernel_main_thread();

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

void * keyboard_echo_thread() {
  text_output_printf("Starting keyboard_echo_thread.\n");

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

  thread_exit();
  return NULL;
}

// Initialization that needs a threaded context is done here
void * kernel_main_thread() {
  // Full acpica needs dynamic memory and scheduling
  acpi_enable_acpica();

  // PCI needs APCICA to determine IRQ mappings
  pci_init();

  // Register PCI drivers
  ahci_register();

  // Once we've registered the PCI drivers, enumerate and instantiate PCI drivers
  pci_enumerate_devices();

  // Set up low-priority thread to echo keyboard to screen
  KernelThread *keyboard_thread = thread_create(keyboard_echo_thread, NULL, 1, 1);
  thread_start(keyboard_thread);

  PCIDevice *ahci_device = pci_find_device(0x01, 0x06, 0x01);
  PCIDeviceDriver *driver = &ahci_device->driver;
  driver->execute_command(driver, AHCI_COMMAND_LIST_DEVICES, NULL, 0, NULL, 0);

  uint16_t buffer[256];
  // struct AHCIIdentifyCommand command = { .device_id = 1 };
  // PCIDeviceDriverError error = driver->execute_command(driver, AHCI_COMMAND_IDENTIFY, &command,
  //                                                      sizeof(command), buffer, sizeof(buffer));



  // if (error == PCI_ERROR_NONE) {
  //   text_output_printf("Supports LBA48? %s\n", (buffer[83] & (1 << 10)) > 0 ? "yes" : "no");
  //   text_output_printf("Max LBA: 0x%.4x%.4x%.4x%.4x\n", buffer[103], buffer[102], buffer[101], buffer[100]);
  //   uint64_t byte_capacity = (buffer[100] + ((uint64_t)buffer[101] << 16) + ((uint64_t)buffer[101] << 32) + ((uint64_t)buffer[101] << 48)) * 512;

  //   text_output_printf("Capacity: %d bytes\n", byte_capacity);
  // } else {
  //   text_output_printf("PCI Error: %d\n", error);
  // }

  struct AHCIReadCommand read_command = { .device_id = 1, .address = 0, .block_count = 1 };
  PCIDeviceDriverError error = driver->execute_command(driver, AHCI_COMMAND_READ, &read_command,
                                  sizeof(read_command), buffer, sizeof(buffer));

  if (error == PCI_ERROR_NONE) {
    for (int i = 0; i < 16; ++i) {
      for (int j = 0; j < 16; ++j) {
        text_output_printf("%.2x %.2x ", (uint8_t)buffer[i*16+j], (uint8_t)(buffer[i*16+j] >> 8));
      }
      text_output_printf("\n");
    }
  } else {
    text_output_printf("PCI Error: %d\n", error);
  }  
  

  text_output_set_foreground_color(0x0000FF00);
  text_output_printf("\nKernel initialization complete. Exiting kernel_main_thread.\n\n");
  text_output_set_foreground_color(0x00FFFFFF);

  thread_exit(); // TODO: Maybe make returning do the same thing?
  return NULL;
}
