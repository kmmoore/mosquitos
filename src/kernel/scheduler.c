#include "scheduler.h"
#include "text_output.h"
#include "datastructures/list.h"
#include "util.h"
#include "virtual_memory.h"
#include "gdt.h"


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
} threads[6]; // TODO: Make this dynamic

struct {
  list thread_list;
  list_entry *current_thread;
  uint32_t next_tid;
} scheduler_data;

void scheduler_init() {
  list_init(&scheduler_data.thread_list);

  scheduler_data.next_tid = 0;

  text_output_printf("Scheduler init\n");
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

void scheduler_add_thread(KernelThread *thread){
  (void)thread;
}

extern void scheduler_context_switch(uint64_t *ss_address);

void scheduler_schedule_next() {
  KernelThread *next = &threads[0];

  scheduler_context_switch(&next->ss);
}