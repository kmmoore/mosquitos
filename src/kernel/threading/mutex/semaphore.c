#include "semaphore.h"
#include "../scheduler.h"
#include "../../util.h"
#include "../../memory/kmalloc.h"
#include "../../hardware/timer.h"
#include "../../drivers/text_output.h"

typedef struct {
  list_entry entry; // This must be first

  KernelThread *thread;
} WaitingThread;

void semaphore_init(Semaphore *sema, uint64_t initial_value) {
  sema->value = initial_value;
  list_init(&sema->waiting_threads);
}

void semaphore_up(Semaphore *sema, uint64_t value) {
  bool interrupts_enabled = interrupts_status();
  cli();
  
  sema->value += value;

  // We have to wake all threads potentially, in case the first one can't be
  // satisfied, but a later one can
  // TODO: Figure out a better way to do this
  WaitingThread *waiting_thread = (WaitingThread *)list_head(&sema->waiting_threads);
  while (waiting_thread) {
    list_remove(&sema->waiting_threads, &waiting_thread->entry);
    thread_wake(waiting_thread->thread);
    kfree(waiting_thread);

    waiting_thread = (WaitingThread *)list_next(&waiting_thread->entry);
  }
  
  // Only re-enable interrupts if they were enabled before
  if (interrupts_enabled) sti();
}

bool semaphore_down(Semaphore *sema, uint64_t value, int64_t timeout) {
  bool interrupts_enabled = interrupts_status();
  cli();

  while (sema->value < value) {
    if (timeout == 0) return false;

    WaitingThread *waiting_thread = kmalloc(sizeof(WaitingThread));
    assert(waiting_thread);

    waiting_thread->thread = scheduler_current_thread();
    list_push_front(&sema->waiting_threads, &waiting_thread->entry); // TODO: Sort by priority
    if (timeout == -1) {
      thread_sleep(waiting_thread->thread);
    } else {
      uint64_t old_value = sema->value;
      timer_thread_sleep(timeout);

      // We woke up either from the timer, or the semaphore
      if (sema->value != old_value) {
        // Semaphore wakeup, waiting_thread has been kfree'd

        if (sema->value >= value) {
          timer_cancel_thread_sleep(scheduler_current_thread());
          break;
        }
      } else {
        // Timer wakeup, destroy waiting_thread and return
        list_remove(&sema->waiting_threads, &waiting_thread->entry);
        kfree(waiting_thread);

        if (interrupts_enabled) sti();
        return false; // Did not get semaphore
      }
    }
  }
  
  // If we get here, we should be able to decrement the semaphore
  assert(sema->value > 0);
  sema->value -= value;

  // Only re-enable interrupts if they were enabled before
  if (interrupts_enabled) sti();

  return true; // Got semaphore
}

uint64_t semaphore_value(Semaphore *sema) {
  return sema->value;
}

