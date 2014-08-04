#include "keyboard_controller.h"
#include "text_output.h"
#include "util.h"
#include "apic.h"
#include "interrupt.h"

#define KEYBOARD_IRQ 1
#define KEYBOARD_IV 0x21

static bool shift_down = false;

static uint8_t SCAN_CODE_MAPPING[] = "\x00""\x1B""1234567890-=""\x08""\tqwertyuiop[]\n\0asdfghjkl;'`\0\\zxcvbnm,./\0*\0 \0\0\0\0\0\0\0\0\0\0\0\0\0-456+1230.\0\0\0\0\0";
static uint8_t SCAN_CODE_MAPPING_SHIFTED[] = "\x00""\x1B""!@#$%^&*()_+""\x08""\tQWERTYUIOP{}\n\0ASDFGHJKL:\"~\0|ZXCVBNM<>?\0*\0 \0\0\0\0\0\0\0\0\0\0\0\0\0789-456+1230.\0\0\0\0\0";

// TODO: Make this a little better
void keyboard_isr() {
  // uint8_t status = inb(0x64);
  uint8_t scancode = inb(0x60); // Read scancode

  // Reset keyboard controller
  // TODO: This apparently isn't necessary, figure out more about it
  uint8_t a = inb(0x61);
  a |= 0x82;
  outb(0x61, a);
  a &= 0x7f;
  outb(0x61, a);

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
        text_output_putchar(pressed_char);
      }
    }
  }

  apic_send_eoi();
}

void keyboard_controller_init() {
  // Map keyboard interrupt
  ioapic_map(KEYBOARD_IRQ, KEYBOARD_IV);
  interrupts_register_handler(KEYBOARD_IV, keyboard_isr);
}
