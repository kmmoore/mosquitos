#include "semaphore.h"
#include "../../util.h"
#include "../../memory/kmalloc.h"
#include "..//scheduler.h"

typedef struct {
  list_entry entry; // This must be first

  KernelThread *thread;
} WaitingThread;

void semaphore_init(Semaphore *sema, uint64_t initial_value) {
  sema->value = initial_value;
  list_init(&sema->waiting_threads);
}

void semaphore_up(Semaphore *sema, uint64_t value) {
  cli();
  
  sema->value++;

  WaitingThread *waiting_thread = (WaitingThread *)list_head(&sema->waiting_threads);
  if (waiting_thread) {
    list_remove(&sema->waiting_threads, &waiting_thread->entry);
    thread_wake(waiting_thread->thread);
    kfree(waiting_thread);
  }

  sti();
}

void semaphore_down(Semaphore *sema, uint64_t value) {
  cli();

  if (sema->value == 0) {
    WaitingThread *waiting_thread = kmalloc(sizeof(WaitingThread));
    assert(waiting_thread);

    waiting_thread->thread = scheduler_current_thread();
    list_push_front(&sema->waiting_threads, &waiting_thread->entry); // TODO: Sort by priority
    thread_sleep(waiting_thread->thread);
  }
  
  assert(sema->value > 0);
  sema->value--;

  sti();
}

uint64_t semaphore_value(Semaphore *sema) {
  return sema->value;
}

