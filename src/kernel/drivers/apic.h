#include <kernel/kernel_common.h>

#ifndef _APIC_H
#define _APIC_H

typedef enum {
  APIC_DIV_1   = 0b1011,
  APIC_DIV_2   = 0b0000,
  APIC_DIV_4   = 0b0001,
  APIC_DIV_8   = 0b0010,
  APIC_DIV_16  = 0b0011,
  APIC_DIV_32  = 0b1000,
  APIC_DIV_64  = 0b1001,
  APIC_DIV_128 = 0b1010,
} APICTimerDivider;

typedef enum {
  APIC_TIMER_ONE_SHOT = 0,
  APIC_TIMER_PERIODIC = 1
} APICTimerMode;

void apic_init();
void apic_send_eoi();
void apic_setup_local_timer(APICTimerDivider divider, uint8_t interrupt_vector, APICTimerMode mode, uint32_t initial_count);
void apic_set_local_timer_masked(bool masked);
void ioapic_map(uint8_t irq_index, uint8_t idt_index);

#endif
