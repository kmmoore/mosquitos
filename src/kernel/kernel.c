#include <efi.h>
#include <efilib.h>

#include "kernel.h"
#include "text_output.h"
#include "gdt.h"
#include "interrupts.h"
#include "pic.h"
#include "keyboard_controller.h"

#include "util.h"

int kernel_main(uint8_t *memory_map, UINTN mem_map_size, UINTN mem_map_descriptor_size, EFI_GRAPHICS_OUTPUT_PROTOCOL *gop) {

  (void)memory_map;
  (void)mem_map_size;
  (void)mem_map_descriptor_size;

  text_output_init(gop);

  text_output_clear_screen(0x00000000);
  text_output_print("Text output initialized!\n");

  // Initialize subsystems
  gdt_init();
  interrupts_init();
  pic_init();

  keyboard_controller_init();

  // Now that interrupt handlers are set up, we can enable interrupts
  sti();

  while (1); // Prevent the kernel from returning

  ASSERT(false); // We should never get here

  return 123;
}