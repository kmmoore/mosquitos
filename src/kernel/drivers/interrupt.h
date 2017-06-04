#include <kernel/kernel_common.h>

#ifndef _INTERRUPTS_H
#define _INTERRUPTS_H

#define SCHEDULER_TIMER_IV 33
#define KEYBOARD_IV 35
#define PIC_TIMER_IV 36
#define PCI_IV 37
// Something is weird about IV 38...
#define LOCAL_APIC_CALIBRATION_IV 39

void interrupt_init();
void interrupt_register_handler(int index, void (*handler)(int));

#endif
