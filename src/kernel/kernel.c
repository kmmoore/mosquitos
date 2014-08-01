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

int kernel_main(KernelInfo info);
void * thread_main(void *);

int kernel_main(KernelInfo info) {

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

  scheduler_init();

  scheduler_create_thread(thread_main);
  scheduler_schedule_next();
  text_output_printf("asdf");

  // Now that interrupt/exception handlers are set up, we can enable interrupts
  sti();

  while (1) {
    __asm__ ("hlt"); // Prevent the kernel from returning
  }

  assert(false); // We should never get here

  return 123;
}

void * thread_main(void *p) {
  (void)p;
  uint64_t rsp;
  __asm__ ("mov %%rsp, %0" : "=r" (rsp));
  text_output_printf("From a thread! 0x%x\n", rsp);
  while (1);
  return NULL;
}