#include "util.h"
#include "text_output.h"

#include <math.h>

int int2str(uint64_t n, char *buf, int buf_len) {

  int len = 0;
  // Compute ceil(log10(n))
  for (int tmp = n; tmp > 0; tmp /= 10, len++);
  if (len + 1 > buf_len) return -1;

  buf[len] = '\0';
  for (int i = len-1; i >= 0; --i) {
    buf[i] = (n % 10) + '0';
    n /= 10;
  }

  return 0;
}

uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ( "inb %1, %0" : "=a"(ret) : "Nd"(port) );
    /* TODO: Is it wrong to use 'N' for the port? It's not a 8-bit constant. */
    /* TODO: Should %1 be %w1? */
    /* TODO: Is there any reason to force the use of eax and edx? */
    return ret;
}

void outb(uint16_t port, uint8_t val) {
  __asm__ volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );
  /* TODO: Is it wrong to use 'N' for the port? It's not a 8-bit constant. */
  /* TODO: Should %1 be %w1? */
  /* TODO: Is there any reason to force the use of eax and edx? */
}