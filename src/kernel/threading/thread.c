#include "thread.h"
#include "scheduler.h"
#include "../util.h"

#include "../memory/virtual_memory.h"
#include "../memory/kmalloc.h"

#include "../hardware/gdt.h"
#include "../hardware/timer.h"
#include "../drivers/text_output.h"

struct KernelThread {
  // NOTE: If the following fields are changed, scheduler.s
  // MUST be updated.
  // DO NOT ADD FIELDS BEFORE THESE ONES

  // Registers popped off by iret
  uint64_t ss, rsp, rflags, cs, rip;

  // Other registers
  uint64_t rax, rbx, rcx, rdx, rsi, rdi, rbp;
  uint64_t r8, r9, r10, r11, r12, r13, r14, r15;
  uint64_t ds, es, fs, gs;

  // These fields can be modified at will

  list_entry entry;
  uint32_t tid;
  uint32_t waiting_on:8; // Number of mutexes, IOs, etc. this thread is waiting on
  uint32_t priority:5;
  uint32_t status:8;
  uint32_t reserved:11;

  uint64_t stack_num_pages;
};

static struct {
  uint32_t next_tid;
} thread_data = { .next_tid = 1 };

KernelThread * thread_create(KernelThreadMain main_func, void * parameter, uint8_t priority, uint64_t stack_num_pages) {
  // Allocate large region for thread struct and stack
  KernelThread *new_thread = vm_palloc(stack_num_pages);

  list_entry_set_value(&new_thread->entry, (uint64_t)new_thread); // Make list entry point to struct

  new_thread->tid = thread_data.next_tid++;
  new_thread->priority = priority;
  new_thread->waiting_on = 0;
  new_thread->stack_num_pages = stack_num_pages;
  new_thread->status = THREAD_SLEEPING;

  // Setup entry point
  new_thread->rip = (uint64_t)main_func;
  new_thread->rdi = (uint64_t)parameter;

  // Setup stack at end of region allocated for thread
  new_thread->rsp = (uint64_t) ((uint8_t *)new_thread + 4096*2);
  new_thread->rbp = new_thread->rsp;

  // Setup flags and segments
  new_thread->cs = GDT_KERNEL_CS;
  new_thread->ss = new_thread->ds = new_thread->es = new_thread->fs = new_thread->gs = GDT_KERNEL_DS;
  new_thread->rflags = 0x202;

  // Setup general purpose registers
  new_thread->rax = new_thread->rbx = new_thread->rcx = new_thread->rdx = new_thread->rsi = new_thread->rdi = 0;
  new_thread->r8 = new_thread->r9 = new_thread->r10 = new_thread->r11 = new_thread->r12 = 0;
  new_thread->r13 = new_thread->r14 = new_thread->r15 = 0;

  return new_thread;
}

uint32_t thread_id(KernelThread *thread) {
  return thread->tid;
}

uint8_t thread_priority(KernelThread *thread) {
  return thread->priority;
}

bool thread_can_run(KernelThread *thread) {
  return thread->waiting_on == 0;
}

uint8_t thread_status(KernelThread *thread) {
  return thread->status;
}

list_entry * thread_list_entry (KernelThread *thread) {
  return &thread->entry;
}

KernelThread * thread_from_list_entry (list_entry *entry) {
  return list_entry_cast(entry, KernelThread *);
}

uint64_t * thread_register_list_pointer (KernelThread *thread) {
  return &thread->ss;
}

void thread_exit() {
  KernelThread *current_thread = scheduler_current_thread();

  cli();
  scheduler_remove_thread(current_thread);

  vm_pfree(current_thread, current_thread->stack_num_pages);
  sti();

  // TODO: There is a race if the scheduler comes in right now
  scheduler_yield_no_save(); 
}

void thread_sleep(KernelThread *thread) {
  // NOTE: This function must be called with interrupts disabled

  thread->status = THREAD_SLEEPING;

  ++thread->waiting_on;

  if (thread->waiting_on == 1) {
    scheduler_remove_thread(thread);
  }

  sti(); // We need interrupts to get scheduling
  scheduler_yield(); // This doesn't return until we wake up
  cli(); // Leave interrupts off (like we found them)
}

void thread_start(KernelThread *thread) {
  thread->status = THREAD_RUNNING;

  scheduler_register_thread(thread);
}

void thread_wake(KernelThread *thread) {
  // NOTE: This function must be called with interrupts disabled
  // text_output_printf("Waking thread: %d\n", thread->tid);

  if (thread->status == THREAD_RUNNING) return;
  
  assert(thread->waiting_on > 0);

  thread->status = THREAD_RUNNING;

  --thread->waiting_on;

  if (thread->waiting_on == 0) {
    scheduler_register_thread(thread);
  }
}