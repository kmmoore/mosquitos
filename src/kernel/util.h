#include "kernel_common.h"

#ifndef _UTIL_H
#define _UTIL_H

#define assert(condition) do { if (!(condition)) panic("assert(" STR(condition) ")"); } while(0)

#define panic(...) do { _panic("PANIC (" __FILE__ ":" STR(__LINE__) ") " __VA_ARGS__); } while (0)

void _panic(char *format, ...);

int int2str(uint64_t n, char *buf, int buf_len, int radix);

uint8_t io_read_8(unsigned port);
void io_write_8(unsigned port, uint8_t val);
uint16_t io_read_16(unsigned port);
void io_write_16(unsigned port, uint16_t val);
uint32_t io_read_32(unsigned port);
void io_write_32(unsigned port, uint32_t val);

void write_msr(uint64_t index, uint64_t value);
uint64_t read_msr(uint64_t index);

void sti();
void cli();

#endif