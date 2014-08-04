#include <efi.h>
#include <efilib.h>

#include "kernel.h"
#include "text_output.h"
#include "acpi.h"
#include "interrupts.h"
#include "exceptions.h"
#include "timer.h"
#include "keyboard_controller.h"
#include "virtual_memory.h"
#include "scheduler.h"

#include "util.h"

#include "../common/build_info.h"

void * thread1_main(void *p) {
  (void)p;
  uint64_t rsp;
  __asm__ ("mov %%rsp, %0" : "=r" (rsp));
  text_output_printf("From thread 1! 0x%x\n", rsp);
  for (uint64_t i = 0; i < 0xafffffff; ++i) {
    __asm__ volatile ("nop");
  }
  text_output_printf("From thread 1.1! 0x%x\n", rsp);
  while(1);
  return NULL;
}

void * thread2_main(void *p) {
  (void)p;
  uint64_t rsp;
  __asm__ ("mov %%rsp, %0" : "=r" (rsp));
  text_output_printf("From thread 2! 0x%x\n", rsp);
  for (uint64_t i = 0; i < 0xafffffff; ++i) {
    __asm__ volatile ("nop");
  }
  text_output_printf("From thread 2.1! 0x%x\n", rsp);
  while(1);
  return NULL;
}

void kernel_main(KernelInfo info) {

  cli();

  text_output_init(info.gop);

  text_output_clear_screen(0x00000000);
  text_output_printf("MosquitOS -- A tiny, annoying operating system\n");
  text_output_printf("Built from %s on %s\n\n", build_git_info, build_time);

  // Initialize subsystems
  acpi_init(info.xdsp_address);
  interrupts_init();
  exceptions_init();

  vm_init(info.memory_map, info.mem_map_size, info.mem_map_descriptor_size);

  timer_init();
  keyboard_controller_init();

  // Now that interrupt/exception handlers are set up, we can enable interrupts
  sti();

  // Set up scheduler
  scheduler_init();

  // KernelThread *t1 = scheduler_create_thread(thread1_main, NULL, 31);
  // KernelThread *t2 = scheduler_create_thread(thread2_main, NULL, 30);
  // scheduler_register_thread(t1);
  // scheduler_register_thread(t2);
  scheduler_start_scheduling();

  while(1);

  assert(false); // We should never get here
}