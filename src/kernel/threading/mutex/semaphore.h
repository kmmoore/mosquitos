#include "../thread.h"
#include "../../kernel_common.h"
#include "../../datastructures/list.h"

#ifndef _SEMAPHORE_H
#define _SEMAPHORE_H

typedef struct {
  uint64_t value;
  list waiting_threads;
} Semaphore;

void semaphore_init(Semaphore *sema, uint64_t initial_value);
void semaphore_up(Semaphore *sema, uint64_t value);
void semaphore_down(Semaphore *sema, uint64_t value);
uint64_t semaphore_value(Semaphore *sema);

#endif
