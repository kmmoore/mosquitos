#include <efi.h>
#include <efilib.h>

#include "kernel.h"
#include "text_output.h"
#include "acpi.h"
#include "interrupts.h"
#include "exceptions.h"
#include "timer.h"
#include "keyboard_controller.h"
#include "virtual_memory.h"

#include "util.h"

int kernel_main(KernelInfo info) {

  text_output_init(info.gop);

  text_output_clear_screen(0x00000000);
  text_output_print("Text output initialized!\n");

  // Initialize subsystems
  acpi_init(info.xdsp_address);
  interrupts_init();
  exceptions_init();

  vm_init(info.memory_map, info.mem_map_size, info.mem_map_descriptor_size);

  timer_init();
  keyboard_controller_init();

  // Now that interrupt/exception handlers are set up, we can enable interrupts
  sti();

  while (1) {
    __asm__ ("hlt"); // Prevent the kernel from returning
  }

  ASSERT(false); // We should never get here

  return 123;
}