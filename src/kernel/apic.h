#include "kernel_common.h"

#ifndef _APIC_H
#define _APIC_H

void apic_init();
void apic_send_eoi();
void ioapic_map(uint8_t irq_index, uint8_t idt_index);

#endif