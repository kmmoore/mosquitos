#include <kernel/threading/mutex/lock.h>

void lock_init(Lock *lock) {
  semaphore_init(&lock->sema, 0);
}

bool lock_acquire(Lock *lock, int64_t timeout) {
  return semaphore_down(&lock->sema, 1, timeout);
}

void lock_release(Lock *lock) {
  semaphore_up(&lock->sema, 1);
}





void spinlock_init(SpinLock *lock) {
  lock->value = 0;
}

void spinlock_acquire(SpinLock *lock) {
  // TODO: Determine the appropriate memory model (currently the strictest)
  while (__sync_lock_test_and_set(&lock->value, 1));
}

void spinlock_release(SpinLock *lock) {
  // TODO: Determine the appropriate memory model (currently the strictest)
  __sync_lock_release(&lock->value);
}
