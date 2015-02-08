#include <kernel/drivers/graphics.h>

static struct {
  EFI_GRAPHICS_OUTPUT_PROTOCOL *gop;
} graphics_data;

void graphics_init(EFI_GRAPHICS_OUTPUT_PROTOCOL *gop) {
  graphics_data.gop = gop;
}