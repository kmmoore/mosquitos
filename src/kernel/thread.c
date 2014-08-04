#include "thread.h"
#include "scheduler.h"
#include "virtual_memory.h"
#include "gdt.h"

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
  uint32_t can_run:1;
  uint32_t priority:5;
  uint32_t reserved:26;
};

static struct {
  KernelThread threads[16]; // TODO: Make this dynamic
  uint32_t next_tid;
} thread_data;

KernelThread * thread_create(KernelThreadMain main_func, void * parameter, uint8_t priority) {
  KernelThread *new_thread = &thread_data.threads[thread_data.next_tid]; // TODO: Make this dynamic

  list_entry_set_value(&new_thread->entry, (uint64_t)new_thread); // Make list entry point to struct

  new_thread->tid = thread_data.next_tid++;
  new_thread->priority = priority;
  new_thread->can_run = true;

  // Setup entry point
  new_thread->rip = (uint64_t)main_func;
  new_thread->rdi = (uint64_t)parameter;

  // Setup stack
  void *stack = vm_palloc(2);
  new_thread->rsp = (uint64_t) ((uint8_t *)stack + 4096*2);
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
  return thread->can_run;
}

list_entry * thread_list_entry (KernelThread *thread) {
  return &thread->entry;
}

KernelThread * thread_from_list_entry (list_entry *entry) {
  return (KernelThread *)list_entry_value(entry);
}

uint64_t * thread_register_list_pointer (KernelThread *thread) {
  return &thread->ss;
}

void thread_exit() {
  scheduler_destroy_thread(scheduler_current_thread());
}
