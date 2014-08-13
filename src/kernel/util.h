#include "kernel_common.h"

#ifndef _UTIL_H
#define _UTIL_H

// From: http://en.wikipedia.org/wiki/Offsetof
#define container_of(ptr, type, member) ({ \
                const typeof( ((type *)0)->member ) *__mptr = (ptr); \
                (type *)( (char *)__mptr - offsetof(type,member) );})

#define member_size(type, member) sizeof(((type *)0)->member)

#define NUM_BITS(bytes) (bytes * 8)
#define ALL_ONES (~0)
#define BOTTOM_N_BITS_OFF(n) (ALL_ONES << n)
#define BOTTOM_N_BITS_ON(n) (~BOTTOM_N_BITS_OFF(n))
#define FIELD_MASK(bit_size, bit_offset) (BOTTOM_N_BITS_ON(bit_size) << bit_offset)

#define field_in_word(word, byte_offset, byte_size) ((word & FIELD_MASK(NUM_BITS(byte_size), NUM_BITS(byte_offset))) >> NUM_BITS(byte_offset))

#define assert(condition) do { if (!(condition)) panic("assert(" STR(condition) ")"); } while(0)

#define panic(...) do { _panic("PANIC (" __FILE__ ":" STR(__LINE__) ") " __VA_ARGS__); } while (0)

void _panic(char *format, ...);

void print_stack_trace();

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
bool interrupts_status();

#endif