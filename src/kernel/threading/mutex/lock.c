#include "lock.h"

void lock_init(Lock *lock) {
  semaphore_init(&lock->sema);
}

void lock_acquire(Lock *lock) {
  semaphore_down(&lock->sema);
}

void lock_release(Lock *lock) {
  semaphore_up(&lock->sema);
}
