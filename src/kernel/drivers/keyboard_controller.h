#include <kernel/kernel_common.h>

#ifndef _KEYBOARD_CONTROLLER_H
#define _KEYBOARD_CONTROLLER_H

void keyboard_controller_init();
int keyboard_controller_read_char(bool block);

#endif