#include "../../kernel_common.h"
#include "semaphore.h"

#ifndef _LOCK_H
#define _LOCK_H

typedef struct {
  Semaphore sema;
} Lock;

void lock_init(Lock *lock);
void lock_acquire(Lock *lock);
void lock_release(Lock *lock);

#endif
