#ifndef _UTIL_H
#define _UTIL_H

#include <kernel/kernel_common.h>

// From: http://en.wikipedia.org/wiki/Offsetof
#define container_of(ptr, type, member)                \
  ({                                                   \
    const typeof(((type *)0)->member) *__mptr = (ptr); \
    (type *)((char *)__mptr - offsetof(type, member)); \
  })

#define member_size(type, member) sizeof(((type *)0)->member)

#define NUM_BITS(bytes) ((bytes)*CHAR_BIT)
#define ALL_ONES (~0ll)
#define BOTTOM_N_BITS_OFF(n) (ALL_ONES << (n))
#define BOTTOM_N_BITS_ON(n) (~BOTTOM_N_BITS_OFF(n))
#define FIELD_MASK(bit_size, bit_offset) \
  (BOTTOM_N_BITS_ON(bit_size) << (bit_offset))

#define ITEM_COUNT(array) (sizeof(array) / sizeof(*array))

#define field_in_word(word, byte_offset, byte_size)                   \
  (((word)&FIELD_MASK(NUM_BITS(byte_size), NUM_BITS(byte_offset))) >> \
   NUM_BITS(byte_offset))

// From:
// http://www.cocoawithlove.com/2008/04/using-pointers-to-recast-in-c-is-bad.html
#define UNION_CAST(x, destType) \
  (((union {                    \
     __typeof__(x) a;           \
     destType b;                \
   })x).b)

#ifdef DEBUG
#define assert(condition)                                  \
  do {                                                     \
    if (!(condition)) panic("assert(" STR(condition) ")"); \
  } while (0)
#else
#define assert(condition)
#endif

#define NOT_IMPLEMENTED panic("NOT IMPLEMENTED");

#define panic(...)                                                  \
  do {                                                              \
    _panic("PANIC (" __FILE__ ":" STR(__LINE__) "): " __VA_ARGS__); \
  } while (0)

void _panic(char *format, ...);

void print_stack_trace();

int int2str(uint64_t n, char *buf, int buf_len, int radix);

int isdigit(int c);
int isxdigit(int c);

int abs(int i);
long int strtol(const char *s, char **endptr, int base);

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
