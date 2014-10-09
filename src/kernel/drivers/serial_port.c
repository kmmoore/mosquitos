// Sends serial data over COM1

#include "serial_port.h"
#include "../drivers/text_output.h"
#include "../util.h"

#define SERIAL_MAX_BAUD_RATE            115200
#define SERIAL_BAUD_RATE                38400

#define SERIAL_COM1_BASE                0x3F8      /* COM1 base port */

#define SERIAL_DATA_PORT(base)          (base)
#define SERIAL_FIFO_COMMAND_PORT(base)  (base + 2)
#define SERIAL_LINE_COMMAND_PORT(base)  (base + 3)
#define SERIAL_MODEM_COMMAND_PORT(base) (base + 4)
#define SERIAL_LINE_STATUS_PORT(base)   (base + 5)

/* SERIAL_LINE_ENABLE_DLAB:
 * Tells the serial port to expect first the highest 8 bits on the data port,
 * then the lowest 8 bits will follow
 */
#define SERIAL_LINE_ENABLE_DLAB         0x80

static void serial_port_configure_baud_rate(unsigned short com, unsigned short divisor) {
  io_write_8(SERIAL_LINE_COMMAND_PORT(com), SERIAL_LINE_ENABLE_DLAB);
  io_write_8(SERIAL_DATA_PORT(com), (divisor >> 8) & 0x00FF);
  io_write_8(SERIAL_DATA_PORT(com), divisor & 0x00FF);
}

static void serial_port_configure_line(unsigned short com) {
  /* Bit:     | 7 | 6 | 5 4 3 | 2 | 1 0 |
   * Content: | d | b | prty  | s | dl  |
   * Value:   | 0 | 0 | 0 0 0 | 0 | 1 1 | = 0x03
   */
  io_write_8(SERIAL_LINE_COMMAND_PORT(com), 0x03);
}

static void serial_port_configure_buffers(unsigned short com) {
  // Enable FIFO, clear them, with 14-byte threshold
  io_write_8(SERIAL_FIFO_COMMAND_PORT(com), 0xC7);
}

static void serial_port_configure_modem(unsigned short com) {
  // Set RTS and DTS (no interrupts because we don't receive)
  io_write_8(SERIAL_MODEM_COMMAND_PORT(com), 0x0B);
}

void serial_port_init() {
  serial_port_configure_baud_rate(SERIAL_COM1_BASE, SERIAL_MAX_BAUD_RATE / SERIAL_BAUD_RATE);
  serial_port_configure_line(SERIAL_COM1_BASE);
  serial_port_configure_buffers(SERIAL_COM1_BASE);
  serial_port_configure_modem(SERIAL_COM1_BASE);
}

static bool serial_port_is_transmit_fifo_empty(unsigned int com) {
  // 5th bit is set when the transmit FIFO is empty
  return (io_read_8(SERIAL_LINE_STATUS_PORT(com)) & 0x20) > 0;
}

void serial_port_putchar(const char c) {
  // while (is_transmit_empty() == 0);
  while (!serial_port_is_transmit_fifo_empty(SERIAL_COM1_BASE));


  io_write_8(SERIAL_DATA_PORT(SERIAL_COM1_BASE), c);
}