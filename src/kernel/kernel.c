#include <efi.h>
#include <efilib.h>

#include "kernel.h"
#include "text_output.h"
#include "acpi.h"
#include "interrupt.h"
#include "exception.h"
#include "timer.h"
#include "keyboard_controller.h"
#include "virtual_memory.h"
#include "scheduler.h"
#include "kmalloc.h"
#include "../common/mem_util.h"
#include "util.h"

#include "../common/build_info.h"

void * thread2_main(void *p UNUSED) {
  text_output_printf("[2] Started!\n");
  text_output_printf("[2] Exiting!\n");
  thread_exit();
  return NULL;
}

void * thread1_main(void *p UNUSED) {
  text_output_printf("[1] Started!\n");
  text_output_printf("[1] Spawning thread 2...\n");

  KernelThread *t2 = thread_create(thread2_main, NULL, 31, 2);
  scheduler_register_thread(t2);

  thread_sleep(4000);
  text_output_printf("[1] Woke up\n");

  thread_exit();
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

  KernelThread *t1 = thread_create(thread1_main, NULL, 20, 2);
  scheduler_register_thread(t1);

  scheduler_start_scheduling(); // kernel_main will not execute any more after this call

  assert(false); // We should never get here
}