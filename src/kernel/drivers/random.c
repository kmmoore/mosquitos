#include <kernel/drivers/random.h>
#include <kernel/drivers/text_output.h>

#include <kernel/drivers/cpuid.h>

static struct {
  bool have_rdseed, have_tsc;
  uint64_t state;
} random_data;

uint64_t random_read_rdseed();

uint64_t random_read_tsc() {
  uint64_t low, high;
  __asm__ volatile("rdtsc" : "=a"(low), "=d"(high));
  return (high << 32) | low;
}

void random_reseed() {
  int iterations = 0;
  do {
    if (random_data.have_rdseed) {
      random_data.state = random_read_rdseed();
    } else {
      random_data.state = 0;
    }

    if (random_data.have_tsc) {
      random_data.state ^= random_read_tsc();
    }

    iterations++;
  } while (random_data.state == 0 && iterations < 10);

  assert(random_data.state != 0);
}

// Implementation of xorshift* with period 2^64 - 1
uint64_t random_random() {
  random_data.state ^= random_data.state >> 12;
  random_data.state ^= random_data.state << 25;
  random_data.state ^= random_data.state >> 27;
  return random_data.state * 0x2545F4914F6CDD1D;
}

void random_init() {
  REQUIRE_MODULE("cpuid");
  random_data.have_rdseed = cpuid_has_extended_capability(CPUID_EXCAP_RDSEED);
  random_data.have_tsc = cpuid_has_capability(CPUID_CAP_TSC);
  assert(random_data.have_rdseed || random_data.have_tsc);
  if (!random_data.have_rdseed) {
    text_output_printf(
        "WARNING: RDSEED not available, random seed quality will suffer.\n");
  }

  random_reseed();
  REGISTER_MODULE("random");
}
