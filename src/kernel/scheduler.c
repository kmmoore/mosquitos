#include "scheduler.h"
#include "text_output.h"
#include "datastructures/list.h"
#include "util.h"
#include "virtual_memory.h"
#include "gdt.h"
#include "apic.h"
#include "timer.h"
#include "interrupts.h"


struct KernelThread {
  uint32_t tid;
  uint32_t can_run:1;
  uint32_t priority:5;
  uint32_t reserved:26;

  // NOTE: If the following fields are changed, scheduler.s
  // MUST be update.

  // Registers popped off by iret
  uint64_t ss, rsp, rflags, cs, rip;

  // Other registers
  uint64_t rax, rbx, rcx, rdx, rsi, rdi, rbp;
  uint64_t r8, r9, r10, r11, r12, r13, r14, r15;
  uint64_t ds, es, fs, gs;
} threads[16]; // TODO: Make this dynamic

#define SCHEDULER_TIMER_CALIBRATION_IV 35
#define SCHEDULER_TIMER_IV 36

#define SCHEDULER_TIMER_CALIBRATION_PERIOD 0x08ffffff
#define SCHEDULER_TIMER_DIVIDER APIC_DIV_2
#define SCHEDULER_TIME_SLICE_MS 1000

struct {
  KernelThread *current_thread;
  list thread_list;
  uint32_t next_tid;
  uint64_t apic_timer_frequency;
} scheduler_data;

static volatile uint64_t calibration_end = 0;
static void apic_timer_calibration_isr() {
  calibration_end = timer_ticks();
  apic_send_eoi();
}

static void calibrate_apic_timer() {
  apic_setup_local_timer(SCHEDULER_TIMER_DIVIDER, SCHEDULER_TIMER_CALIBRATION_IV, APIC_TIMER_ONE_SHOT, SCHEDULER_TIMER_CALIBRATION_PERIOD);
  interrupts_register_handler(SCHEDULER_TIMER_CALIBRATION_IV, apic_timer_calibration_isr);

  text_output_printf("Calibrating APIC timer...");

  uint64_t calibration_start = timer_ticks();
  apic_set_local_timer_masked(false);

  // Spin until we get the APIC timer interrupt
  while(calibration_end == 0);

  scheduler_data.apic_timer_frequency = (uint64_t)SCHEDULER_TIMER_CALIBRATION_PERIOD * TIMER_FREQUENCY / (calibration_end - calibration_start);

  text_output_printf("Done - frequency: %dHz\n", scheduler_data.apic_timer_frequency);
}

void setup_scheduler_timer() {
  uint32_t period = scheduler_data.apic_timer_frequency * SCHEDULER_TIME_SLICE_MS / 1000;
  // text_output_printf("Period: %d\n", period);
  apic_setup_local_timer(SCHEDULER_TIMER_DIVIDER, SCHEDULER_TIMER_IV, APIC_TIMER_PERIODIC, period);
  apic_set_local_timer_masked(false);
}

void scheduler_init() {
  list_init(&scheduler_data.thread_list);

  scheduler_data.next_tid = 0;

  calibrate_apic_timer();

  text_output_printf("Scheduler init\n");
}

void scheduler_load_thread(uint64_t *ss_pointer);
void scheduler_start_scheduling(KernelThread *initial_thread) {
  scheduler_data.current_thread = initial_thread;
  setup_scheduler_timer();
  // TODO: There is a race condition between these lines
  scheduler_load_thread(&initial_thread->ss);
}

KernelThread * scheduler_create_thread(KernelThreadMain main_func, void * parameter, uint8_t priority) {
  KernelThread *new_thread = &threads[scheduler_data.next_tid]; // TODO: Make this dynamic

  new_thread->tid = scheduler_data.next_tid++;
  new_thread->priority = priority;

  // Setup entry point
  new_thread->rip = (uint64_t)main_func;
  new_thread->rdi = (uint64_t)parameter;

  // Setup stack
  void *stack = vm_palloc(2);
  new_thread->rsp = (uint64_t) ((uint8_t *)stack + 4096*2);
  text_output_printf("thread rsp: 0x%x\n", new_thread->rsp);
  new_thread->rbp = new_thread->rsp;

  // Setup flags and segments
  new_thread->cs = GDT_KERNEL_CS;
  new_thread->ss = new_thread->ds = new_thread->es = new_thread->fs = new_thread->gs = GDT_KERNEL_DS;
  new_thread->rflags = 0x202;

  // Setup general purpose registers
  new_thread->rax = new_thread->rbx = new_thread->rcx = new_thread->rdx = new_thread->rsi = new_thread->rdi = 0;
  new_thread->r8 = new_thread->r9 = new_thread->r10 = new_thread->r11 = new_thread->r12 = 0;
  new_thread->r13 = new_thread->r14 = new_thread->r15 = 0;

  return &threads[0];
}

extern void scheduler_load_thread(uint64_t *ss_address);

void scheduler_set_next() {
  uint32_t next_tid = scheduler_data.current_thread->tid == 0 ? 1 : 0; // Switch between first two threads
  scheduler_data.current_thread = &threads[next_tid];
}