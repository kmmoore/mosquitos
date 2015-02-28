#include <kernel/drivers/keyboard_controller.h>
#include <kernel/util.h>

#include <kernel/drivers/text_output.h>
#include <kernel/drivers/apic.h>
#include <kernel/drivers/interrupt.h>

#include <kernel/threading/mutex/lock.h>

#include <kernel/datastructures/queue.h>

#include <limits.h>

#define KEYBOARD_IRQ 1
#define KEYBOARD_IV 0x21

static struct {
  bool shift_down;
  Queue *input_queue;
  Lock lock;
} keyboard_data;

static uint8_t SCAN_CODE_MAPPING[] = "\x00""\x1B""1234567890-=""\x08""\tqwertyuiop[]\n\0asdfghjkl;'`\0\\zxcvbnm,./\0*\0 \0\0\0\0\0\0\0\0\0\0\0\0\0-456+1230.\0\0\0\0\0";
static uint8_t SCAN_CODE_MAPPING_SHIFTED[] = "\x00""\x1B""!@#$%^&*()_+""\x08""\tQWERTYUIOP{}\n\0ASDFGHJKL:\"~\0|ZXCVBNM<>?\0*\0 \0\0\0\0\0\0\0\0\0\0\0\0\0789-456+1230.\0\0\0\0\0";

// TODO: Make this a little better
void keyboard_isr() {
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
    if (scancode == 0x2a || scancode == 0x36) keyboard_data.shift_down = false;
  } else {
    if (scancode == 0x2a || scancode == 0x36) {
      keyboard_data.shift_down = true;
    } else {
      char pressed_char;
      if (keyboard_data.shift_down) pressed_char = SCAN_CODE_MAPPING_SHIFTED[scancode];
      else pressed_char = SCAN_CODE_MAPPING[scancode];

      QueueValue v = { .i = pressed_char };

      queue_enqueue(keyboard_data.input_queue, v, true);
    }
  }
}

int keyboard_controller_read_char(bool block) {
  assert(!block); // Not implemented yet

  int return_value = INT_MIN;
  if (queue_count(keyboard_data.input_queue) > 0) {
    QueueValue v = queue_dequeue(keyboard_data.input_queue);
    return_value = v.i;
  }

  return return_value;

}

void keyboard_controller_init() {
  REQUIRE_MODULE("interrupt");
  REQUIRE_MODULE("text_output");

  keyboard_data.shift_down = false;
  keyboard_data.input_queue = queue_alloc(1024);
  lock_init(&keyboard_data.lock);

  // Map keyboard interrupt
  interrupt_register_handler(KEYBOARD_IV, keyboard_isr);
  ioapic_map(KEYBOARD_IRQ, KEYBOARD_IV);

  REGISTER_MODULE("keyboard_controller");
}
