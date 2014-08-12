#include "timer.h"
#include "../drivers/text_output.h"
#include "apic.h"
#include "interrupt.h"
#include "../util.h"
#include "../datastructures/list.h"
#include "../memory/kmalloc.h"
#include "../threading/scheduler.h"

#define TIMER_IRQ 2
#define TIMER_IV 0x22

struct waiting_thread {
  list_entry entry; // This must be first

  KernelThread *thread;
  uint64_t wake_time;
};

static struct {
  volatile uint64_t ticks; // Won't overflow for 5e8 ticks
  list waiting_threads;
} timer_data;

static inline void wake_waiting_thread(struct waiting_thread *wt) {
  list_remove(&timer_data.waiting_threads, (list_entry *)wt);
  thread_wake(wt->thread);
  kfree(wt);
}

void timer_isr() {
  ++timer_data.ticks;

  struct waiting_thread *current = (struct waiting_thread *)list_head(&timer_data.waiting_threads);
  while (current) {
    struct waiting_thread *next = (struct waiting_thread *)list_next((list_entry *)current);
    if (timer_data.ticks >= current->wake_time) {
      wake_waiting_thread(current);
    }

    current = next;
  }

  apic_send_eoi();
}

uint64_t timer_ticks() {
  return timer_data.ticks;
}

void timer_init() {

  text_output_printf("Initializing timer...");

  timer_data.ticks = 0;
  list_init(&timer_data.waiting_threads);

  ioapic_map(TIMER_IRQ, TIMER_IV);
  interrupts_register_handler(TIMER_IV, timer_isr);

  // Use Legacy PIC Timer
  // TODO: Use HPET eventually
  io_write_8(0x43, 0x68);

  // Set up timer to have frequency TIMER_FREQUENCY
  io_write_8(0x40, TIMER_DIVIDER & 0xff);
  io_write_8(0x40, TIMER_DIVIDER >> 8);

  text_output_printf("Done\n");
}

void timer_thread_sleep(uint64_t milliseconds) {
  KernelThread *thread = scheduler_current_thread();

  struct waiting_thread *new_entry = kmalloc(sizeof(struct waiting_thread));

  new_entry->thread = thread;

  uint64_t ticks = milliseconds * 1000 / TIMER_FREQUENCY;
  new_entry->wake_time = timer_data.ticks + ticks;

  bool interrupts_enabled = interrupts_status();
  cli();

  list_push_front(&timer_data.waiting_threads, &new_entry->entry);

  // This won't return until the thread wakes up
  // NOTE: Interrupts will be disabled when it returns
  thread_sleep(thread);

  // Only re-enable interrupts if they were enabled before
  if (interrupts_enabled) sti();

}

void timer_cancel_thread_sleep(KernelThread *thread) {
  bool interrupts_enabled = interrupts_status();
  cli();

  struct waiting_thread *current = (struct waiting_thread *)list_head(&timer_data.waiting_threads);
  while (current) {
    if (current->thread == thread) {
      wake_waiting_thread(current);
    }
    current = (struct waiting_thread *)list_next(&current->entry);
  }

  // Only re-enable interrupts if they were enabled before
  if (interrupts_enabled) sti();
}

