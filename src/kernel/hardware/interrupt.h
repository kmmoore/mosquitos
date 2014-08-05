#include "../kernel_common.h"

#ifndef _INTERRUPTS_H
#define _INTERRUPTS_H

void interrupts_init();
void interrupts_register_handler(int index, void (*handler)(int));

#endif