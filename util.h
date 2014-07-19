#include <stdint.h>

#ifndef _UTIL_H
#define _UTIL_H

int int2str(uint64_t n, char *buf, int buf_len);
uint8_t inb(uint16_t port);
void outb(uint16_t port, uint8_t val);

#endif