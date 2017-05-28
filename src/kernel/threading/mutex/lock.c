#include <kernel/threading/mutex/lock.h>

// TODO: Add memory barriers
void lock_init(Lock *lock) {
  semaphore_init(&lock->sema, 1);
}

bool lock_acquire(Lock *lock, int64_t timeout) {
  assert(semaphore_value(&lock->sema) <= 1);
  return semaphore_down(&lock->sema, 1, timeout);
}

void lock_release(Lock *lock) {
  assert(semaphore_value(&lock->sema) == 0);
  semaphore_up(&lock->sema, 1);
}





void spinlock_init(SpinLock *lock) {
  lock->value = 0;
}

void spinlock_acquire(SpinLock *lock) {
  // TODO: Determine the appropriate memory model (currently the strictest)
  // TODO: This is probably very broken, I think it will never let a lower
  // priority thread run while the lock is held
  while (__sync_lock_test_and_set(&lock->value, 1));
}

void spinlock_release(SpinLock *lock) {
  // TODO: Determine the appropriate memory model (currently the strictest)
  __sync_lock_release(&lock->value);
}
