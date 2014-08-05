#include <efi.h>
#include <efilib.h>

#include "font.h"
#include "text_output.h"
#include "../util.h"

#define kTextOutputPadding 10

static struct {
  EFI_GRAPHICS_OUTPUT_PROTOCOL *gop;
  int current_row, current_col;
} text_output;

void text_output_init(EFI_GRAPHICS_OUTPUT_PROTOCOL *gop) {
  text_output.gop = gop;

  text_output.current_row = text_output.current_col = kTextOutputPadding;
}

static void text_output_draw_pixel(int x, int y, uint32_t color) {
  ((uint32_t *)text_output.gop->Mode->FrameBufferBase)[y * text_output.gop->Mode->Info->HorizontalResolution + x] = color;
}

void text_output_clear_screen(uint32_t color) {
  for (unsigned int x = 0; x < text_output.gop->Mode->Info->HorizontalResolution; ++x) {
    for (unsigned int y = 0; y < text_output.gop->Mode->Info->VerticalResolution; y++) {
      text_output_draw_pixel(x, y, color);
    }
  }
}

static void text_output_draw_char(char c, int x, int y) {
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

void text_output_backspace() {
  if (text_output.current_col == kTextOutputPadding) { // Beginning of line
    // TODO: Figure out how to go back up...
  } else {
    text_output.current_col -= 1;
    // Blank out character
    text_output_draw_char(' ', text_output.current_col, text_output.current_row);
  }
}

inline void text_output_putchar(char c) {
  if (c == '\n') {
    text_output.current_row += 2; // Put an empty line between text lines
    text_output.current_col = kTextOutputPadding;
  } else{
    text_output_draw_char(c, text_output.current_col, text_output.current_row);
    text_output.current_col += 1;
  }
}

void text_output_print(char *str) {
  while (*str != '\0') {
    text_output_putchar(*str);
    str++;
  }
}

void text_output_printf(char *format, ...) {
  va_list arg_list;
  va_start(arg_list, format);

  text_output_vprintf(format, arg_list);

  va_end(arg_list);
}

void text_output_vprintf(char *format, va_list arg_list) {
  char int_conv_buffer[21]; // Can hold a 64-bit decimal string with null termination

  while (*format) {
    if (*format == '%') {
      format++;

      switch (*format) {
        case 'd':
        {
          int64_t number = va_arg(arg_list, int64_t);
          int2str(number, int_conv_buffer, sizeof(int_conv_buffer), 10);
          text_output_print(int_conv_buffer);
        }
        break;

        case 'x':
        {
          uint64_t number = va_arg(arg_list, uint64_t);
          int2str(number, int_conv_buffer, sizeof(int_conv_buffer), 16);
          text_output_print(int_conv_buffer);
        }
        break;

        case 'b':
        {
          uint64_t number = va_arg(arg_list, uint64_t);
          int2str(number, int_conv_buffer, sizeof(int_conv_buffer), 2);
          text_output_print(int_conv_buffer);
        }
        break;

        case 'c':
        {
          char c = va_arg(arg_list, int);
          text_output_putchar(c);
        }
        break;

        case 's':
        {
          char *string = va_arg(arg_list, char *);
          text_output_print(string);
        }
        break;
      }

    } else {
      text_output_putchar(*format);
    }
    format++;
  }
}
