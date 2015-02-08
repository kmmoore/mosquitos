#include <kernel/kernel_common.h>

#ifndef _SERIAL_PORT_H_
#define _SERIAL_PORT_H_

void serial_port_init();
void serial_port_putchar(const char c);

#endif // _SERIAL_PORT_H_