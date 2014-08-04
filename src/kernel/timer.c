#include "timer.h"
#include "text_output.h"
#include "apic.h"
#include "interrupt.h"
#include "util.h"

#define TIMER_IRQ 2
#define TIMER_IV 0x22

static volatile uint64_t ticks = 0; // Won't overflow for 5e8

void timer_isr() {
  ++ticks;

  apic_send_eoi();
}

uint64_t timer_ticks() {
  return ticks;
}

void timer_init() {

  text_output_printf("Initializing timer...");

  ioapic_map(TIMER_IRQ, TIMER_IV);
  interrupts_register_handler(TIMER_IV, timer_isr);

  // Use Legacy PIC Timer
  // TODO: Use HPET eventually
  outb(0x43, 0x68);

  // Set up timer to have frequency TIMER_FREQUENCY
  outb(0x40, TIMER_DIVIDER & 0xff);
  outb(0x40, TIMER_DIVIDER >> 8);

  text_output_printf("Done\n");
}