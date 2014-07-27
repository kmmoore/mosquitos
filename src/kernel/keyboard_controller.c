#include <efi.h>
#include <efilib.h>

#include <stdbool.h>

#include "keyboard_controller.h"
#include "text_output.h"
#include "util.h"
#include "pic.h"
#include "interrupts.h"

static bool shift_down = false;

static uint8_t SCAN_CODE_MAPPING[] = "\x00""\x1B""1234567890-=""\x08""\tqwertyuiop[]\n\0asdfghjkl;'`\0\\zxcvbnm,./\0*\0 \0\0\0\0\0\0\0\0\0\0\0\0\0-456+1230.\0\0\0\0\0";
static uint8_t SCAN_CODE_MAPPING_SHIFTED[] = "\x00""\x1B""!@#$%^&*()_+""\x08""\tQWERTYUIOP{}\n\0ASDFGHJKL:\"~\0|ZXCVBNM<>?\0*\0 \0\0\0\0\0\0\0\0\0\0\0\0\0789-456+1230.\0\0\0\0\0";

void keyboard_isr() {
  // uint8_t status = inb(0x64);
  uint8_t scancode = inb(0x60); // Read scancode

  if (scancode & 0x80) {
    scancode = scancode & ~0x80;
    if (scancode == 0x2a || scancode == 0x36) shift_down = false;
  } else {
    if (scancode == 0x2a || scancode == 0x36) shift_down = true;
    char buf[2] = { 0, 0 };

    if (shift_down) buf[0] = SCAN_CODE_MAPPING_SHIFTED[scancode];
    else buf[0] = SCAN_CODE_MAPPING[scancode];

    text_output_print(buf);
  }

  outb(0x20, 0x20); // Acknowledge interrupt
}

extern void gdt_flush();

void keyboard_controller_init() {
  outb(0x21,0b11111101); // Enable keyboard IRQ
  outb(0xa1,0xff);

  interrupts_register_handler(0x21, keyboard_isr);
}
