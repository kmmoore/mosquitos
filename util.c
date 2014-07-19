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
