#include "timer.h"
#include "text_output.h"
#include "apic.h"
#include "interrupts.h"
#include "util.h"

#define TIMER_IRQ 2
#define TIMER_IV 0x22

volatile uint64_t milliseconds = 0; // Won't overflow for 5e8

void timer_isr() {
  ++milliseconds;

  apic_send_eoi();
}

uint64_t timer_millis() {
  return milliseconds;
}

void timer_init() {

  text_output_printf("Initializing timer...");

  ioapic_map(TIMER_IRQ, TIMER_IV);
  interrupts_register_handler(TIMER_IV, timer_isr);

  // Use Legacy PIC Timer
  // TODO: Use HPET eventually
  outb(0x43, 0x68);

  // Set up 1000.151Hz timer (1 tick/ms)
  outb(0x40, 0xa9);
  outb(0x40, 0x04);

  text_output_printf("Done\n");
}