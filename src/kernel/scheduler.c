#include "scheduler.h"
#include "text_output.h"
#include "datastructures/list.h"
#include "util.h"
#include "virtual_memory.h"

struct KernelThread {
  uint32_t tid;
  uint32_t can_run:1;
  uint32_t priority:5;
  uint32_t reserved:26;

  uint64_t rsp, rip, rax, rbx, rcx, rdx, rsi, rdi, rbp;
  uint64_t r8, r9, r10, r11, r12, r13, r14, r15;
  uint64_t ss, cs, ds, es, fs, gs, rflags;
} threads[6]; // TODO: Make this dynamic

struct {
  list thread_list;
  list_entry *current_thread;
} scheduler_data;

void scheduler_init() {
  list_init(&scheduler_data.thread_list);
  text_output_printf("Scheduler init\n");
}

KernelThread * scheduler_create_thread(KernelThreadMain main_func) {
  threads[0].rip = (uint64_t)main_func;

  void *stack = vm_palloc(2);
  threads[0].rsp = (uint64_t) ((uint8_t *)stack + 4096*2);
  text_output_printf("thread rsp: 0x%x\n", threads[0].rsp);
  threads[0].rbp = threads[0].rsp;
  threads[0].cs = 0x08;
  threads[0].ss = threads[0].ds = threads[0].es = threads[0].fs = threads[0].gs = 0x10;

  threads[0].rflags = 0x202;
  return &threads[0];
}

void scheduler_add_thread(KernelThread *thread){
  (void)thread;
}

void scheduler_schedule_next() {
  KernelThread *next = &threads[0];

  __asm__ ("push %0" : : "m" (next->ss));
  __asm__ ("push %0" : : "m" (next->rsp));
  __asm__ ("push %0" : : "m" (next->rflags));
  __asm__ ("push %0" : : "m" (next->cs));
  __asm__ ("push %0" : : "m" (next->rip));

  __asm__ ("mov %0, %%rbp" : : "m" (next->rbp) : "rbp");
  __asm__ ("mov $0, %%rax" : : : "rax");
  __asm__ ("mov $0, %%rbx" : : : "rbx");
  __asm__ ("mov $0, %%rcx" : : : "rcx");
  __asm__ ("mov $0, %%rdx" : : : "rdx");
  __asm__ ("mov $0, %%rsi" : : : "rsi");
  __asm__ ("mov $0, %%rdi" : : : "rdi");

  __asm__ ("iretq");
}