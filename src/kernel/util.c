#include "util.h"
#include "drivers/text_output.h"

#include <stdarg.h>

void _panic(char *format, ...) {
  va_list arg_list;
  va_start(arg_list, format);

  text_output_vprintf(format, arg_list);

  va_end(arg_list);

  __asm__ ("cli \n\t hlt");
}

int int2str(uint64_t n, char *buf, int buf_len, int radix) {
  static char *digit_lookup = "0123456789abcdef";

  // Compute log base `radix` of the number
  int len = 0;
  for (uint64_t tmp = n; tmp > 0; tmp /= radix, len++);

  // Special case for zero
  if (n == 0) len = 1;

  if (len + 1 > buf_len) return -1; // Don't overflow

  buf[len] = '\0';
  for (int i = len-1; i >= 0; --i) {
    buf[i] = digit_lookup[(n % radix)];
    n /= radix;
  }

  return 0;
}

void sti() {
  __asm__ ("sti");
}

void cli() {
  __asm__ ("cli");
}

uint8_t io_read_8(unsigned port) {
    uint8_t ret;
    __asm__ volatile ("inb %w1, %b0" : "=a" (ret) : "Nd" (port));
    return ret;
}

void io_write_8(unsigned port, uint8_t val) {
  __asm__ volatile ("outb %b0, %w1" : : "a" (val), "Nd" (port));
}

uint16_t io_read_16(unsigned port) {
    uint8_t ret;
    __asm__ volatile ("inw %w1, %w0" : "=a" (ret) : "Nd" (port));
    return ret;
}

void io_write_16(unsigned port, uint16_t val) {
  __asm__ volatile ("outw %w0, %w1" : : "a" (val), "Nd" (port));
}

uint32_t io_read_32(unsigned port) {
    uint8_t ret;
    __asm__ volatile ("inl %w1, %k0" : "=a" (ret) : "Nd" (port));
    return ret;
}

void io_write_32(unsigned port, uint32_t val) {
  __asm__ volatile ("outl %k0, %w1" : : "a" (val), "Nd" (port));
}

void write_msr(uint64_t index, uint64_t value) {
  uint64_t high = value >> 32;
  uint64_t low  = (value & 0x0000000000000000ffffffffffffffff);
  __asm__ ("rdmsr" : : "a" (low), "d" (high), "c" (index));
}

uint64_t read_msr(uint64_t index) {
  uint64_t high, low;
  __asm__ ("rdmsr" : "=a" (low), "=d" (high) : "c" (index));

  return high << 32 | low;
}