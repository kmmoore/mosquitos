#include "scheduler.h"
#include "thread.h"
#include "../util.h"

#include "../hardware/apic.h"
#include "../hardware/timer.h"
#include "../hardware/interrupt.h"

#include "../drivers/text_output.h"
#include "../datastructures/list.h"

#define SCHEDULER_TIMER_CALIBRATION_IV 35
#define SCHEDULER_TIMER_IV 36
#define SCHEDULER_YIELD_NO_SAVE_IV 37

#define SCHEDULER_TIMER_CALIBRATION_PERIOD 0x0ffffff
#define SCHEDULER_TIMER_DIVIDER APIC_DIV_2
#define SCHEDULER_TIME_SLICE_MS 10

struct {
  KernelThread *current_thread; // This must be the first entry

  list thread_list; // Sorted list (by priority) used as a priority queue
  uint64_t apic_timer_frequency;
} scheduler_data;

static volatile uint64_t calibration_end = 0;
static void apic_timer_calibration_isr() {
  calibration_end = timer_ticks();
  apic_send_eoi();
}

static void calibrate_apic_timer() {
  // Setup a one-shot timer
  apic_setup_local_timer(SCHEDULER_TIMER_DIVIDER, SCHEDULER_TIMER_CALIBRATION_IV, APIC_TIMER_ONE_SHOT, SCHEDULER_TIMER_CALIBRATION_PERIOD);
  interrupts_register_handler(SCHEDULER_TIMER_CALIBRATION_IV, apic_timer_calibration_isr);

  text_output_printf("Calibrating APIC timer...");

  uint64_t calibration_start = timer_ticks();
  apic_set_local_timer_masked(false);

  // Spin until we get the APIC timer interrupt
  while(calibration_end == 0);

  // Determine APIC frequency pased on number of PIC ticks that happened
  scheduler_data.apic_timer_frequency = (uint64_t)SCHEDULER_TIMER_CALIBRATION_PERIOD * TIMER_FREQUENCY / (calibration_end - calibration_start);

  text_output_printf("Done - frequency: %dHz\n", scheduler_data.apic_timer_frequency);
}

static void setup_scheduler_timer() {
  uint32_t period = scheduler_data.apic_timer_frequency * SCHEDULER_TIME_SLICE_MS / 1000;
  apic_setup_local_timer(SCHEDULER_TIMER_DIVIDER, SCHEDULER_TIMER_IV, APIC_TIMER_PERIODIC, period);
  apic_set_local_timer_masked(false);
}

void * idle_thread_main(void *p UNUSED) {
  while (1) __asm__ ("hlt");
  return NULL;
}

void scheduler_init() {
  list_init(&scheduler_data.thread_list);

  scheduler_data.current_thread = NULL;

  // The idle thread is the thread that runs if we have nothing else to do
  KernelThread *idle_thread = thread_create(idle_thread_main, NULL, 0, 1);
  scheduler_register_thread(idle_thread);

  calibrate_apic_timer();

  text_output_printf("Scheduler init\n");
}

void scheduler_set_next() {
  // Attempt to use next thread in list to round-robin
  // schedule all top priority threads

  KernelThread *next = NULL;
  list_entry *first_thread_entry = list_head(&scheduler_data.thread_list);
  KernelThread *first_thread = thread_from_list_entry(first_thread_entry);

  // Schedule the next thread in line if there isn't a higher priority thread waiting
  if (scheduler_data.current_thread &&
      thread_priority(scheduler_data.current_thread) >= thread_priority(first_thread)) {

    list_entry *current_thread_entry = thread_list_entry(scheduler_data.current_thread);
    list_entry *next_thread_entry = list_next(current_thread_entry);

    if (next_thread_entry) next = thread_from_list_entry(next_thread_entry);
  }
  
  // If we can't use the next one, pull highest priority ready thread off
  if (!next || thread_priority(next) < thread_priority(scheduler_data.current_thread)) {
    list_entry *entry = list_head(&scheduler_data.thread_list);

    while (entry) {
      next = thread_from_list_entry(entry);
      if (thread_can_run(next)) break;

      entry = list_next(entry);
    }
  }

  // Set the next thread
  scheduler_data.current_thread = next;
}

void scheduler_load_thread(uint64_t *ss_pointer);
void scheduler_start_scheduling() {
  scheduler_set_next();

  setup_scheduler_timer();
  // TODO: There is a race condition between these lines, but it shouldn't be an issue
  // because the thread loading should happen so much faster than the first clock tick
  scheduler_load_thread(thread_register_list_pointer(scheduler_data.current_thread));
}

void scheduler_register_thread(KernelThread *thread) {
  // Put the new thread in the priority queue
  list_entry *current = list_head(&scheduler_data.thread_list);

  // Try to find a place to put the new thread
  while (true) {
    KernelThread *t = thread_from_list_entry(current);
    if (thread_priority(thread) >= thread_priority(t)) break;
    current = list_next(current);
  }

  // If we couldn't find a place to put it, put it at the end
  if (current) {
    list_insert_before(&scheduler_data.thread_list, current, thread_list_entry(thread));
  } else {
    list_push_back(&scheduler_data.thread_list, thread_list_entry(thread));
  }
}

KernelThread * scheduler_current_thread() {
  return scheduler_data.current_thread;
}

void scheduler_yield() {
  // Yield and save the current thread
  __asm__ ("int $" STR(SCHEDULER_TIMER_IV));
}

void scheduler_yield_no_save() {
  // Yield without saving the current thread
  __asm__ ("int $" STR(SCHEDULER_YIELD_NO_SAVE_IV));
}

void scheduler_remove_thread(KernelThread *thread) {
  list_entry *current_entry = thread_list_entry(thread);

  list_remove(&scheduler_data.thread_list, current_entry);
}