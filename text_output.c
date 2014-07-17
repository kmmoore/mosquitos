#include <efi.h>
#include <efilib.h>

#include "font.h"
#include "text_output.h"

static struct {
  EFI_GRAPHICS_OUTPUT_PROTOCOL *gop;
  int current_row, current_col;
  uint32_t font[kNumCharsInFont * kCharacterHeight];
} text_output;

void text_output_init(EFI_GRAPHICS_OUTPUT_PROTOCOL *gop) {
  text_output.gop = gop;

  text_output.current_row = text_output.current_col = 0;

  store_font((uint32_t *)&text_output.font);
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

  if (c >= 'a' && c <= 'z') {
    c = c - 'a' + 'A';
  }

  int font_char_index;

  if (c >= '0' && c <= '9') {
    font_char_index = 26 + c - '0';
  } else if (c == ' ') {
    font_char_index = 36;
  } else {
    font_char_index = c - 'A';
  }

  for (int i = 0; i < kCharacterWidth; ++i) {
    for (int j = 0; j < kCharacterHeight; ++j) {
      uint32_t color = (text_output.font[j + font_char_index * kCharacterHeight] & (1 << (7-i))) == 0 ? 0x0 : 0x00ffffff;
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