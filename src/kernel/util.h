#include "kernel_common.h"

#ifndef _UTIL_H
#define _UTIL_H

int int2str(uint64_t n, char *buf, int buf_len, int radix);

uint8_t inb(uint16_t port);
void outb(uint16_t port, uint8_t val);

void sti();
void cli();

#endif