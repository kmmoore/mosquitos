#include "../kernel_common.h"
#include "../thread.h"
#include "../datastructures/list.h"

#ifndef _SEMAPHORE_H
#define _SEMAPHORE_H

typedef struct {
  uint64_t value;
  list waiting_threads;
} Semaphore;

void semaphore_init(Semaphore *sema);
void semaphore_up(Semaphore *sema);
void semaphore_down(Semaphore *sema);
uint64_t semaphore_value(Semaphore *sema);

#endif
