#include <kernel/drivers/graphics.h>

static struct {
  EFI_GRAPHICS_OUTPUT_PROTOCOL *gop;
  uint32_t *frame_buffer_base;
  uint32_t pixels_per_line;
} graphics_data;

void graphics_init(EFI_GRAPHICS_OUTPUT_PROTOCOL *gop) {
  graphics_data.gop = gop;
  graphics_data.frame_buffer_base = (uint32_t *)gop->Mode->FrameBufferBase;
  graphics_data.pixels_per_line = gop->Mode->Info->PixelsPerScanLine;

  REGISTER_MODULE("graphics");
}

void graphics_clear_screen(uint32_t color) {
  graphics_fill_rect(0, 0, graphics_data.gop->Mode->Info->HorizontalResolution,
                     graphics_data.gop->Mode->Info->VerticalResolution, color);
}

void graphics_fill_rect(int x, int y, int w, int h, uint32_t color) {
  for (int i = y; i < y + h; ++i) {
    for (int j = x; j < x + w; ++j) {
      graphics_draw_pixel(i, j, color);
    }
  }
}

void graphics_draw_pixel(int x, int y, uint32_t color) {
  graphics_data.frame_buffer_base[y * graphics_data.pixels_per_line + x] =
      color;
}
