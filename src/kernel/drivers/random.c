#include <kernel/drivers/random.h>

#include <kernel/drivers/text_output.h>

static struct { uint64_t state; } random_data;

uint64_t random_read_rdseed();

uint64_t random_read_tsc() {
  uint64_t low, high;
  __asm__ volatile("rdtsc" : "=a"(low), "=d"(high));
  return (high << 32) | low;
}

void random_reseed() {
  do {
    random_data.state = random_read_rdseed();
    random_data.state ^= random_read_tsc();
  } while (random_data.state == 0);
}

// Implementation of xorshift* with period 2^64 - 1
uint64_t random_random() {
  random_data.state ^= random_data.state >> 12;
  random_data.state ^= random_data.state << 25;
  random_data.state ^= random_data.state >> 27;
  return random_data.state * 0x2545F4914F6CDD1D;
}

void random_init() {
  random_reseed();
  REGISTER_MODULE("random");
}
