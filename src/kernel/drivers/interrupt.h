#include "../kernel_common.h"

#ifndef _INTERRUPTS_H
#define _INTERRUPTS_H

void interrupt_init();
void interrupt_register_handler(int index, void (*handler)(int));

#endif