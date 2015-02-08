#include <kernel/kernel_common.h>
#include <kernel/threading/mutex/semaphore.h>

#ifndef _LOCK_H
#define _LOCK_H

typedef struct {
  Semaphore sema;
} Lock;

void lock_init(Lock *lock);
bool lock_acquire(Lock *lock, int64_t timeout); // timeout of -1 means wait forever
void lock_release(Lock *lock);

typedef struct {
  char value;
} SpinLock;

void spinlock_init(SpinLock *lock);
void spinlock_acquire(SpinLock *lock);
void spinlock_release(SpinLock *lock);

#endif
