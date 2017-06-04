#include <kernel/kernel_common.h>

#ifndef _RANDOM_H
#define _RANDOM_H

void random_init();
void random_reseed();
// Returns a (non-cryptographically-secure) random number in [0, 2^64).
uint64_t random_random();

#endif
