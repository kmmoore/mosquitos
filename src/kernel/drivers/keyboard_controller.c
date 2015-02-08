#include <kernel/drivers/keyboard_controller.h>
#include <kernel/util.h>

#include <kernel/drivers/text_output.h>
#include <kernel/drivers/apic.h>
#include <kernel/drivers/interrupt.h>

#define KEYBOARD_IRQ 1
#define KEYBOARD_IV 0x21

static bool shift_down = false;

static uint8_t SCAN_CODE_MAPPING[] = "\x00""\x1B""1234567890-=""\x08""\tqwertyuiop[]\n\0asdfghjkl;'`\0\\zxcvbnm,./\0*\0 \0\0\0\0\0\0\0\0\0\0\0\0\0-456+1230.\0\0\0\0\0";
static uint8_t SCAN_CODE_MAPPING_SHIFTED[] = "\x00""\x1B""!@#$%^&*()_+""\x08""\tQWERTYUIOP{}\n\0ASDFGHJKL:\"~\0|ZXCVBNM<>?\0*\0 \0\0\0\0\0\0\0\0\0\0\0\0\0789-456+1230.\0\0\0\0\0";

// TODO: Make this a little better
void keyboard_isr() {
  // uint8_t status = inb(0x64);
  uint8_t scancode = io_read_8(0x60); // Read scancode

  // Reset keyboard controller
  // TODO: This apparently isn't necessary, figure out more about it
  uint8_t a = io_read_8(0x61);
  a |= 0x82;
  io_write_8(0x61, a);
  a &= 0x7f;
  io_write_8(0x61, a);

  if (scancode & 0x80) {
    scancode = scancode & ~0x80;
    if (scancode == 0x2a || scancode == 0x36) shift_down = false;
  } else {
    if (scancode == 0x2a || scancode == 0x36) {
      shift_down = true;
    } else {
      char pressed_char;
      if (shift_down) pressed_char = SCAN_CODE_MAPPING_SHIFTED[scancode];
      else pressed_char = SCAN_CODE_MAPPING[scancode];

      if (pressed_char == '\b' || pressed_char == 127) {
        text_output_backspace();
      } else {
        uint32_t old_fg_color = text_output_get_foreground_color();
        text_output_set_foreground_color(0x006666FF);
        text_output_putchar(pressed_char);
        text_output_set_foreground_color(old_fg_color);
      }
    }
  }

  apic_send_eoi();
}

void keyboard_controller_init() {
  REQUIRE_MODULE("interrupt");
  REQUIRE_MODULE("text_output");

  // Map keyboard interrupt
  interrupt_register_handler(KEYBOARD_IV, keyboard_isr);
  ioapic_map(KEYBOARD_IRQ, KEYBOARD_IV);

  REGISTER_MODULE("keyboard_controller");
}
