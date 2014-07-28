#include <efi.h>
#include <efilib.h>

#include "kernel.h"
#include "text_output.h"
#include "interrupts.h"
#include "exceptions.h"
#include "keyboard_controller.h"
#include "virtual_memory.h"

#include "util.h"

int kernel_main(uint8_t *memory_map, uint64_t mem_map_size, uint64_t mem_map_descriptor_size, EFI_GRAPHICS_OUTPUT_PROTOCOL *gop) {

  text_output_init(gop);

  text_output_clear_screen(0x00000000);
  text_output_print("Text output initialized!\n");

  // Initialize subsystems
  interrupts_init();
  exceptions_init();

  vm_init(memory_map, mem_map_size, mem_map_descriptor_size);

  keyboard_controller_init();

  // Now that interrupt/exception handlers are set up, we can enable interrupts
  sti();

  while (1) {
    // __asm__ ("hlt"); // Prevent the kernel from returning
  }

  ASSERT(false); // We should never get here

  return 123;
}