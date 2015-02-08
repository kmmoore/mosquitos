#include <kernel/threading/thread.h>
#include <kernel/kernel_common.h>
#include <kernel/datastructures/list.h>

#ifndef _SEMAPHORE_H
#define _SEMAPHORE_H

typedef struct {
  uint64_t value;
  List waiting_threads;
} Semaphore;

void semaphore_init(Semaphore *sema, uint64_t initial_value);
void semaphore_up(Semaphore *sema, uint64_t value);
bool semaphore_down(Semaphore *sema, uint64_t value, int64_t timeout);
uint64_t semaphore_value(Semaphore *sema);

#endif
