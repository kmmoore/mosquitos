#include <efi.h>
#include <efilib.h>

#include "../format/format.h"

#include "font.h"
#include "text_output.h"
#include "../drivers/serial_port.h"
#include "../util.h"
#include "../threading/mutex/lock.h"

#define kTextOutputPadding 1

static struct {
  EFI_GRAPHICS_OUTPUT_PROTOCOL *gop;
  int current_row, current_col;
  uint32_t background_color, foreground_color;
  Lock lock;
} text_output_data;


void text_output_init(EFI_GRAPHICS_OUTPUT_PROTOCOL *gop) {
  REQUIRE_MODULE("serial_port");

  // TODO: Sort out locking
  lock_init(&text_output_data.lock);
  
  text_output_data.gop = gop;

  text_output_data.current_row = text_output_data.current_col = kTextOutputPadding;

  text_output_data.foreground_color = 0x00FFFFFF;
  text_output_data.background_color = 0x00000000;

  REGISTER_MODULE("text_output");
}

void text_output_set_background_color(uint32_t color) {
  text_output_data.background_color = color;
}

void text_output_set_foreground_color(uint32_t color) {
  text_output_data.foreground_color = color;
}

uint32_t text_output_get_background_color() {
  return text_output_data.background_color;
}

uint32_t text_output_get_foreground_color() {
  return text_output_data.foreground_color;
}

static void text_output_draw_pixel(int x, int y, uint32_t color) {
  ((uint32_t *)text_output_data.gop->Mode->FrameBufferBase)[y * text_output_data.gop->Mode->Info->PixelsPerScanLine + x] = color;
}

void text_output_clear_screen() {
  // TODO: The graphics stuff should be in a graphics library, not this library
  // TODO: We should use gop->Blt here
  for (unsigned int x = 0; x < text_output_data.gop->Mode->Info->HorizontalResolution; ++x) {
    for (unsigned int y = 0; y < text_output_data.gop->Mode->Info->VerticalResolution; y++) {
      text_output_draw_pixel(x, y, text_output_data.background_color);
    }
  }
  text_output_data.current_row = text_output_data.current_col = kTextOutputPadding;
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
      uint32_t color = (font[j + font_char_index * kCharacterHeight] & (1 << (7-i))) == 0 ? text_output_data.background_color : text_output_data.foreground_color;
      text_output_draw_pixel(pixel_x + i, pixel_y + j, color);
    }
  }
}

void text_output_backspace() {
  // lock_acquire(&text_output_data.lock, -1);

  if (text_output_data.current_col == kTextOutputPadding) { // Beginning of line
    // TODO: Figure out how to go back up...
  } else {
    text_output_data.current_col -= 1;
    // Blank out character
    text_output_draw_char(' ', text_output_data.current_col, text_output_data.current_row);
  }

  serial_port_putchar(0x8); // ASCII backspace character

  // lock_release(&text_output_data.lock);
}

inline void text_output_putchar(const char c) {
  // lock_acquire(&text_output_data.lock, -1);

  if (c == '\n') {
    text_output_data.current_row += 2; // Put an empty line between text lines

    // int max_lines = text_output_data.gop->Mode->Info->VerticalResolution / kCharacterHeight;
    // if (text_output_data.current_row == 100) {
    //   text_output_clear_screen();
    // }

    text_output_data.current_col = kTextOutputPadding;
  } else{
    text_output_draw_char(c, text_output_data.current_col, text_output_data.current_row);
    text_output_data.current_col += 1;
  }

  serial_port_putchar(c);

  // lock_release(&text_output_data.lock);
}

void text_output_print(const char *str) {
  while (*str != '\0') {
    text_output_putchar(*str);
    str++;
  }
}

void * text_output_format_consumer(void *arg UNUSED, const char *buffer, size_t n) {
  while (n--) {
    text_output_putchar(*buffer++);
  }

  return (void *)( !NULL );
}

int text_output_printf(const char *fmt, ...) {
  va_list arg_list;
  va_start(arg_list, fmt);

  int num_chars = text_output_vprintf(fmt, arg_list);

  va_end(arg_list);

  return num_chars;
}

int text_output_vprintf(const char *fmt, va_list arg_list) {
  return format(text_output_format_consumer, NULL, fmt, arg_list);
}

void text_output_safe_printf(const char *fmt, ...) {
  va_list arg_list;
  va_start(arg_list, fmt);

  text_output_safe_vprintf(fmt, arg_list);

  va_end(arg_list);
}

void text_output_safe_vprintf(const char *fmt, va_list arg_list) {
  char int_conv_buffer[21]; // Can hold a 64-bit decimal string with null termination

  while (*fmt) {
    if (*fmt == '%') {

      bool matched_format;
      do {
        fmt++;
        matched_format = true; // Will stay true unless we reach the default case

        switch (*fmt) {
          case '%':
          {
            text_output_putchar('%');
          }
          break;

          case 'd':
          {
            int64_t number = va_arg(arg_list, int64_t);
            if (number < 0) {
              text_output_putchar('-');
              number = abs(number);
            }

            int2str(number, int_conv_buffer, sizeof(int_conv_buffer), 10);
            text_output_print(int_conv_buffer);
          }
          break;

          case 'u':
          {
            uint64_t number = va_arg(arg_list, uint64_t);
            int2str(number, int_conv_buffer, sizeof(int_conv_buffer), 10);
            text_output_print(int_conv_buffer);
          }
          break;

          case 'x':
          case 'X':
          {
            uint32_t number = va_arg(arg_list, uint32_t);
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

          default:
          {
            matched_format = false;
          }
        }

      } while (!matched_format);

    } else {
     text_output_putchar(*fmt);
   }
   fmt++;
 }
}
