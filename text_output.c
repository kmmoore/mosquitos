#include <efi.h>
#include <efilib.h>

#include "font.h"
#include "text_output.h"

static struct {
  EFI_GRAPHICS_OUTPUT_PROTOCOL *gop;
  int current_row, current_col;
} text_output;

void text_output_init(EFI_GRAPHICS_OUTPUT_PROTOCOL *gop) {
  text_output.gop = gop;

  text_output.current_row = text_output.current_col = 0;
}

void text_output_draw_pixel(int x, int y, uint32_t color) {
  ((uint32_t *)text_output.gop->Mode->FrameBufferBase)[y * text_output.gop->Mode->Info->HorizontalResolution + x] = color;
}

void text_output_clear_screen(uint32_t color) {
  for (int x = 0; x < text_output.gop->Mode->Info->HorizontalResolution; ++x) {
    for (int y = 0; y < text_output.gop->Mode->Info->VerticalResolution; y++) {
      text_output_draw_pixel(x, y, color);
    }
  }
}

void text_output_draw_char(char c, int x, int y) {
  int pixel_x = x * kCharacterWidth;
  int pixel_y = y * kCharacterHeight;

  int font_char_index;

  if (c >= ' ' && c <= '~') {
    font_char_index = c - ' ';
  } else {
    return; // We cannot print this character
  }

  for (int i = 0; i < kCharacterWidth; ++i) {
    for (int j = 0; j < kCharacterHeight; ++j) {
      uint32_t color = (font[j + font_char_index * kCharacterHeight] & (1 << (7-i))) == 0 ? 0x0 : 0x00ffffff;
      text_output_draw_pixel(pixel_x + i, pixel_y + j, color);
    }
  }
}

void text_output_draw_string(char *str, int x, int y) {
  while (*str != '\0') {
    text_output_draw_char(*str, x++, y);
    str++;
  }
}