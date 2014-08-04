#include "timer.h"
#include "text_output.h"
#include "apic.h"
#include "interrupt.h"
#include "util.h"
#include "datastructures/list.h"
#include "kmalloc.h"
#include "scheduler.h"

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

void timer_isr() {
  ++timer_data.ticks;

  struct waiting_thread *current = (struct waiting_thread *)list_head(&timer_data.waiting_threads);
  while (current) {
    struct waiting_thread *next = (struct waiting_thread *)list_next((list_entry *)current);
    if (timer_data.ticks >= current->wake_time) {
      list_remove(&timer_data.waiting_threads, (list_entry *)current);
      thread_wake(current->thread);
      kfree(current);
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
  outb(0x43, 0x68);

  // Set up timer to have frequency TIMER_FREQUENCY
  outb(0x40, TIMER_DIVIDER & 0xff);
  outb(0x40, TIMER_DIVIDER >> 8);

  text_output_printf("Done\n");
}

void timer_thread_sleep(uint64_t milliseconds) {
  KernelThread *thread = scheduler_current_thread();

  struct waiting_thread *new_entry = kmalloc(sizeof(struct waiting_thread));

  new_entry->thread = thread;

  uint64_t ticks = milliseconds * 1000 / TIMER_FREQUENCY;
  new_entry->wake_time = timer_data.ticks + ticks;

  cli();

  list_push_front(&timer_data.waiting_threads, &new_entry->entry);

  // This won't return until the thread wakes up
  // NOTE: Interrupts will be disabled when it returns
  thread_sleep(thread);

  sti();

}
