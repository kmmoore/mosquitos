#include <efi.h>
#include <efilib.h>

#include "kernel.h"
#include "text_output.h"

void kernel_main(uint8_t *memory_map, UINTN mem_map_size, UINTN mem_map_descriptor_size, EFI_GRAPHICS_OUTPUT_PROTOCOL *gop) {

  text_output_init(gop);

  text_output_clear_screen(0x00000000);
  text_output_draw_string("Hello world", 10, 10);

  while (1); // Prevent the kernel from returning

  ASSERT(false); // We should never get here
}