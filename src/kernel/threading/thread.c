#include <kernel/threading/scheduler.h>
#include <kernel/threading/thread.h>
#include <kernel/util.h>

#include <kernel/memory/kmalloc.h>
#include <kernel/memory/virtual_memory.h>

#include <kernel/drivers/gdt.h>
#include <kernel/drivers/text_output.h>
#include <kernel/drivers/timer.h>

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

  ListEntry entry;
  uint32_t tid;
  uint32_t
      waiting_on : 8;  // Number of mutexes, IOs, etc. this thread is waiting on
  uint32_t priority : 5;  // Thread priority, higher priority threads will
                          // preempt lower priority threads
  uint32_t status : 8;
  uint32_t reserved : 11;

  uint64_t stack_num_pages;
};

static struct { uint32_t next_tid; } thread_data = {.next_tid = 1};

// Wrapper function that calls thread_exit() when the main_func returns.
static void thread_wrapper(KernelThreadMain main_func, void *parameter) {
  // TODO: Use the return value;
  main_func(parameter);
  thread_exit();
}

KernelThread *thread_create(KernelThreadMain main_func, void *parameter,
                            uint8_t priority, uint64_t stack_num_pages) {
  assert(sizeof(KernelThread) < stack_num_pages * VM_PAGE_SIZE);

  // Allocate large region for thread struct and stack
  KernelThread *new_thread = vm_palloc(stack_num_pages);

  assert(priority < 32);  // We only have 5 bits

  new_thread->tid = thread_data.next_tid++;
  new_thread->priority = priority;
  new_thread->waiting_on = 0;
  new_thread->stack_num_pages = stack_num_pages;
  new_thread->status = THREAD_SLEEPING;

  // Setup entry point
  new_thread->rip = (uint64_t)thread_wrapper;
  new_thread->rdi = (uint64_t)main_func;
  new_thread->rsi = (uint64_t)parameter;

  // Setup stack at end of region allocated for thread
  new_thread->rsp =
      (uint64_t)((uint8_t *)new_thread + stack_num_pages * VM_PAGE_SIZE);
  new_thread->rbp = new_thread->rsp;

  // Setup flags and segments
  new_thread->cs = GDT_KERNEL_CS;
  new_thread->ss = new_thread->ds = new_thread->es = new_thread->fs =
      new_thread->gs = GDT_KERNEL_DS;
  new_thread->rflags = 0x202;

  // Setup general purpose registers (except RDI/RSI which were used above for
  // parameter passing).
  new_thread->rax = new_thread->rbx = new_thread->rcx = new_thread->rdx = 0;
  new_thread->r8 = new_thread->r9 = new_thread->r10 = new_thread->r11 = 0;
  new_thread->r12 = new_thread->r13 = new_thread->r14 = new_thread->r15 = 0;

  return new_thread;
}

uint32_t thread_id(KernelThread *thread) { return thread->tid; }

uint8_t thread_priority(KernelThread *thread) { return thread->priority; }

bool thread_can_run(KernelThread *thread) { return thread->waiting_on == 0; }

uint8_t thread_status(KernelThread *thread) { return thread->status; }

ListEntry *thread_list_entry(KernelThread *thread) { return &thread->entry; }

KernelThread *thread_from_list_entry(ListEntry *entry) {
  return container_of(entry, KernelThread, entry);
}

uint64_t *thread_register_list_pointer(KernelThread *thread) {
  return &thread->ss;
}

void thread_exit() {
  KernelThread *current_thread = scheduler_current_thread();

  cli();
  scheduler_remove_thread(current_thread);

  vm_pfree(current_thread, current_thread->stack_num_pages);
  sti();

  // TODO: There is a race if the scheduler comes in right now
  // TODO: Make setting scheduler->current_thread to NULL cause yield_no_save to
  // be called
  scheduler_yield_no_save();
}

void thread_sleep(KernelThread *thread) {
  // NOTE: This function must be called with interrupts disabled
  assert(!interrupts_status());
  thread->status = THREAD_SLEEPING;

  ++thread->waiting_on;

  if (thread->waiting_on == 1) {
    scheduler_remove_thread(thread);
  }

  sti();              // We need interrupts to get scheduling
  scheduler_yield();  // This doesn't return until we wake up
  cli();              // Leave interrupts off (like we found them)
}

void thread_start(KernelThread *thread) {
  thread->status = THREAD_RUNNING;

  scheduler_register_thread(thread);
}

void thread_wake(KernelThread *thread) {
  // NOTE: This function must be called with interrupts disabled
  assert(!interrupts_status());
  if (thread->status == THREAD_RUNNING) return;

  assert(thread->waiting_on > 0);

  --thread->waiting_on;

  if (thread->waiting_on == 0) {
    thread->status = THREAD_RUNNING;
    scheduler_register_thread(thread);
  }
}
